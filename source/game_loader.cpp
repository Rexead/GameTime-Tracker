#include "gametime.h"
#include <cstring>
#include <ctime>
#include <map>
#include <cstdlib>
#include <memory>
#include <csetjmp>
#include <jpeglib.h>

// Extracts the u64 program_id from a PdmPlayEvent (stored as two u32 words, swapped).
static u64 getEventProgramId(const PdmPlayEvent& ev)
{
    // program_id is stored as two u32 words with high/low swapped:
    // [0] is the high word, [1] is the low word.
    u32 hi = ev.event_data.applet.program_id[0];
    u32 lo = ev.event_data.applet.program_id[1];
    return ((u64)hi << 32) | (u64)lo;
}

// ─── Progress state, read by the UI thread while loadInstalledGames() runs ──
std::atomic<int> g_loadProgress{0};
std::atomic<int> g_loadTotal{0};

namespace {
    // std::thread on devkitPro gets a small default stack, which was
    // overflowing here: NsApplicationControlData is ~136KB and used to live
    // on the stack inside the loop, and libjpeg needs its own scratch space
    // on top of that. We use libnx's native Thread API instead so we can
    // hand it an explicitly large stack.
    Thread             g_loadThreadHandle;
    bool               g_loadThreadValid = false;
    std::atomic<bool>  g_loadDone{false};
    std::vector<GameEntry>* g_loadOut = nullptr;

    constexpr size_t LOAD_THREAD_STACK_SIZE = 512 * 1024; // 512KB, generous margin for libjpeg's internal state

    void loadThreadEntry(void*)
    {
        if (g_loadOut)
            loadInstalledGames(*g_loadOut);
        g_loadDone = true;
    }
}

// ─── Icon decoding ────────────────────────────────────────────────────────────
// NsApplicationControlData stores the icon as raw JPEG bytes for the first
// language entry. We decode it with libjpeg-turbo and box-downscale it to
// ICON_SIZE x ICON_SIZE RGBA8 so the renderer can blit it directly.
struct JpegErrorMgr {
    jpeg_error_mgr pub;
    jmp_buf        jumpBuf;
};

static void jpegErrorExit(j_common_ptr cinfo)
{
    JpegErrorMgr* err = (JpegErrorMgr*)cinfo->err;
    longjmp(err->jumpBuf, 1);
}

static bool decodeIconJpeg(const u8* data, size_t size, std::vector<u32>& outPixels, int targetDim)
{
    if (!data || size == 0) return false;

    jpeg_decompress_struct cinfo;
    JpegErrorMgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.jumpBuf)) {
        // libjpeg longjmp's here on a decode error
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, (unsigned long)size);

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    int srcW    = (int)cinfo.output_width;
    int srcH    = (int)cinfo.output_height;
    int comps   = cinfo.output_components;

    if (srcW <= 0 || srcH <= 0 || comps < 3) {
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    std::vector<u8> full((size_t)srcW * srcH * comps);
    std::vector<u8> rowbuf((size_t)srcW * comps);

    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW row = rowbuf.data();
        jpeg_read_scanlines(&cinfo, &row, 1);
        u32 y = cinfo.output_scanline - 1;
        memcpy(&full[(size_t)y * srcW * comps], rowbuf.data(), (size_t)srcW * comps);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    // Nearest-neighbour downscale into the icon buffer.
    outPixels.resize((size_t)targetDim * targetDim);
    for (int y = 0; y < targetDim; y++) {
        int sy = y * srcH / targetDim;
        for (int x = 0; x < targetDim; x++) {
            int sx = x * srcW / targetDim;
            const u8* p = &full[((size_t)sy * srcW + sx) * comps];
            outPixels[(size_t)y * targetDim + x] = RGBA8(p[0], p[1], p[2], 255);
        }
    }
    return true;
}

static bool fetchDailyPlaytime(u64 titleId, std::vector<DailyPlaytime>& out)
{
    // Get the range of available events
    s32 totalEntries = 0, startIdx = 0, endIdx = 0;
    if (R_FAILED(pdmqryGetAvailablePlayEventRange(&totalEntries, &startIdx, &endIdx)))
        return false;
    if (totalEntries == 0) return true;

    // Read in batches of 256
    static const int BATCH = 256;
    PdmPlayEvent batch[BATCH];

    // Track launch/exit pairs by entry_index
    struct Session { u64 launchTime; };
    std::map<u32, u64> launches; // entry_index -> launch timestamp
    std::map<u64, u64> dayMap;   // YYYYMMDD -> seconds

    s32 idx = startIdx;
    while (idx <= endIdx) {
        s32 got = 0;
        if (R_FAILED(pdmqryQueryPlayEvent(idx, batch, BATCH, &got)) || got == 0)
            break;

        for (int i = 0; i < got; i++) {
            const PdmPlayEvent& ev = batch[i];

            // Only Applet-type events
            if (ev.play_event_type != PdmPlayEventType_Applet) continue;

            // Filter by titleId
            if (getEventProgramId(ev) != titleId) continue;

            u8 etype = ev.event_data.applet.event_type;
            u64 ts   = ev.timestamp_user; // PosixTime (seconds)

            if (etype == PdmAppletEventType_Launch ||
                etype == PdmAppletEventType_InFocus) {
                launches[ev.event_data.applet.program_id[0]] = ts;
            }
            else if (etype == PdmAppletEventType_Exit ||
                     etype == PdmAppletEventType_Exit5 ||
                     etype == PdmAppletEventType_Exit6 ||
                     etype == PdmAppletEventType_OutOfFocus ||
                     etype == PdmAppletEventType_OutOfFocus4) {
                auto it = launches.find(ev.event_data.applet.program_id[0]);
                if (it != launches.end()) {
                    u64 start = it->second;
                    u64 end_  = ts;
                    if (end_ > start) {
                        u64 dur = end_ - start;
                        time_t t = (time_t)start;
                        struct tm* ti = gmtime(&t);
                        if (ti) {
                            u64 key = (u64)(ti->tm_year + 1900) * 10000
                                    + (u64)(ti->tm_mon  + 1)    * 100
                                    + (u64)(ti->tm_mday);
                            dayMap[key] += dur;
                        }
                    }
                    launches.erase(it);
                }
            }
        }
        idx += got;
    }

    out.clear();
    for (auto& [key, secs] : dayMap) {
        DailyPlaytime dp;
        dp.year    = (int)(key / 10000);
        dp.month   = (int)((key / 100) % 100);
        dp.day     = (int)(key % 100);
        dp.seconds = secs;
        out.push_back(dp);
    }

    std::sort(out.begin(), out.end(), [](const DailyPlaytime& a, const DailyPlaytime& b) {
        if (a.year  != b.year)  return a.year  > b.year;
        if (a.month != b.month) return a.month > b.month;
        return a.day > b.day;
    });
    return true;
}

bool loadInstalledGames(std::vector<GameEntry>& out)
{
    out.clear();

    if (R_FAILED(nsInitialize()))     return false;
    if (R_FAILED(pdmqryInitialize())) { nsExit(); return false; }

    AccountUid uid = {};
    bool hasUser = false;
    if (R_SUCCEEDED(accountInitialize(AccountServiceType_Application))) {
        s32 cnt = 0;
        AccountUid users[8];
        if (R_SUCCEEDED(accountListAllUsers(users, 8, &cnt)) && cnt > 0) {
            uid     = users[0];
            hasUser = true;
        }
        accountExit();
    }

    static const int MAX_APPS = 2000;
    NsApplicationRecord records[MAX_APPS];
    s32 recordCount = 0;

    if (R_FAILED(nsListApplicationRecord(records, MAX_APPS, 0, &recordCount))) {
        pdmqryExit(); nsExit();
        return false;
    }

    g_loadTotal    = recordCount;
    g_loadProgress = 0;

    // ~136KB (NACP + icon JPEG buffer). Kept off the stack and reused across
    // iterations so we're not depending on how big the calling thread's
    // stack happens to be.
    static NsApplicationControlData ctrl;

    for (int i = 0; i < recordCount; i++) {
        u64 titleId = records[i].application_id;

        memset(&ctrl, 0, sizeof(ctrl));
        u64 ctrlSize = 0;

        if (R_FAILED(nsGetApplicationControlData(
                NsApplicationControlSource_Storage,
                titleId, &ctrl, sizeof(ctrl), &ctrlSize))) {
            g_loadProgress = i + 1;
            continue;
        }

        // Name — first non-empty language entry
        std::string name;
        for (int lang = 0; lang < 16; lang++) {
            const char* c = ctrl.nacp.lang[lang].name;
            if (c[0] != '\0') {
                name = std::string(c, strlen(c));
                if (name.size() > 512) name.resize(512);
                break;
            }
        }
        if (name.empty()) name = "Unknown Title";

        // Total playtime
        u64 totalSecs = 0;
        if (hasUser) {
            PdmPlayStatistics stats;
            memset(&stats, 0, sizeof(stats));
            if (R_SUCCEEDED(pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(
                    titleId, uid, false, &stats))) {
                // playtime is in nanoseconds on newer libnx versions
                totalSecs = stats.playtime / 1000000000ULL;
                // If the result is 0, this might be the old format (minutes)
                if (totalSecs == 0 && stats.playtime > 0)
                    totalSecs = stats.playtime * 60;
            }
        }

        GameEntry entry;
        entry.titleId      = titleId;
        entry.name         = name;
        entry.totalSeconds = totalSecs;
        entry.hasIcon      = false;

        // Icon: JPEG bytes immediately follow NacpStruct in the control data
        if (ctrlSize > sizeof(ctrl.nacp)) {
            size_t iconSize = (size_t)(ctrlSize - sizeof(ctrl.nacp));
            if (decodeIconJpeg(ctrl.icon, iconSize, entry.icon, ICON_SIZE))
                entry.hasIcon = true;
        }

        // Daily breakdown from the log events
        fetchDailyPlaytime(titleId, entry.dailyLog);

        // If PDM stats came back zero but we have event data, sum from events
        if (entry.totalSeconds == 0 && !entry.dailyLog.empty()) {
            for (auto& d : entry.dailyLog) entry.totalSeconds += d.seconds;
        }

        out.push_back(std::move(entry));
        g_loadProgress = i + 1;
    }

    std::sort(out.begin(), out.end(), [](const GameEntry& a, const GameEntry& b) {
        return a.totalSeconds > b.totalSeconds;
    });

    pdmqryExit();
    nsExit();
    return true;
}

// ─── Background loading API ──────────────────────────────────────────────────

void startGameLoad(std::vector<GameEntry>* out)
{
    g_loadProgress    = 0;
    g_loadTotal       = 0;
    g_loadDone        = false;
    g_loadOut         = out;
    g_loadThreadValid = false;

    Result rc = threadCreate(&g_loadThreadHandle, loadThreadEntry, nullptr,
                              nullptr, LOAD_THREAD_STACK_SIZE, 0x2C, -2);
    if (R_FAILED(rc)) {
        // Couldn't spin up the thread (extremely unlikely) — fall back to
        // loading synchronously so the app doesn't just hang forever.
        loadThreadEntry(nullptr);
        return;
    }

    rc = threadStart(&g_loadThreadHandle);
    if (R_FAILED(rc)) {
        threadClose(&g_loadThreadHandle);
        loadThreadEntry(nullptr);
        return;
    }

    g_loadThreadValid = true;
}

bool isGameLoadDone()
{
    return g_loadDone.load();
}

void finishGameLoad()
{
    if (g_loadThreadValid) {
        threadWaitForExit(&g_loadThreadHandle);
        threadClose(&g_loadThreadHandle);
        g_loadThreadValid = false;
    }
}