#include "gametime.h"
#include <cstring>
#include <cstdio>
#include <cctype>

// ─── Framebuffer state (set at start of each renderFrame call) ───────────────
static u32* gFb    = nullptr;
static u32  gStride = 0;

static inline void px(int x, int y, u32 c)
{
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
    gFb[y * gStride + x] = c;
}

static void hline(int x, int y, int w, u32 c)
{ for (int i = 0; i < w; i++) px(x+i, y, c); }

static void vline(int x, int y, int h, u32 c)
{ for (int i = 0; i < h; i++) px(x, y+i, c); }

static void rect(int x, int y, int w, int h, u32 c)
{ for (int r = 0; r < h; r++) hline(x, y+r, w, c); }

// ─── 5×7 bitmap font ─────────────────────────────────────────────────────────
static const u8 FONT[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x00,0x7F,0x41,0x41,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x00,0x41,0x41,0x7F,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04}, // ^
    {0x40,0x40,0x40,0x40,0x40}, // _
    {0x00,0x01,0x02,0x04,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78}, // a
    {0x7F,0x48,0x44,0x44,0x38}, // b
    {0x38,0x44,0x44,0x44,0x20}, // c
    {0x38,0x44,0x44,0x48,0x7F}, // d
    {0x38,0x54,0x54,0x54,0x18}, // e
    {0x08,0x7E,0x09,0x01,0x02}, // f
    {0x0C,0x52,0x52,0x52,0x3E}, // g
    {0x7F,0x08,0x04,0x04,0x78}, // h
    {0x00,0x44,0x7D,0x40,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00}, // j
    {0x7F,0x10,0x28,0x44,0x00}, // k
    {0x00,0x41,0x7F,0x40,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78}, // m
    {0x7C,0x08,0x04,0x04,0x78}, // n
    {0x38,0x44,0x44,0x44,0x38}, // o
    {0x7C,0x14,0x14,0x14,0x08}, // p
    {0x08,0x14,0x14,0x18,0x7C}, // q
    {0x7C,0x08,0x04,0x04,0x08}, // r
    {0x48,0x54,0x54,0x54,0x20}, // s
    {0x04,0x3F,0x44,0x40,0x20}, // t
    {0x3C,0x40,0x40,0x20,0x7C}, // u
    {0x1C,0x20,0x40,0x20,0x1C}, // v
    {0x3C,0x40,0x30,0x40,0x3C}, // w
    {0x44,0x28,0x10,0x28,0x44}, // x
    {0x0C,0x50,0x50,0x50,0x3C}, // y
    {0x44,0x64,0x54,0x4C,0x44}, // z
    {0x00,0x08,0x36,0x41,0x00}, // {
    {0x00,0x00,0x7F,0x00,0x00}, // |
    {0x00,0x41,0x36,0x08,0x00}, // }
    {0x10,0x08,0x08,0x10,0x08}, // ~
};

static void drawChar(int x, int y, char ch, u32 col, int s)
{
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    const u8* g = FONT[(u8)ch - 0x20];
    for (int col_ = 0; col_ < 5; col_++) {
        u8 bits = g[col_];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                for (int sy = 0; sy < s; sy++)
                    for (int sx = 0; sx < s; sx++)
                        px(x + col_*s + sx, y + row*s + sy, col);
            }
        }
    }
}

static int drawStr(int x, int y, const char* str, u32 col, int s = 2)
{
    while (*str) {
        drawChar(x, y, *str++, col, s);
        x += (5 + 1) * s;
    }
    return x;
}

static int drawStr(int x, int y, const std::string& s_, u32 col, int s = 2)
{ return drawStr(x, y, s_.c_str(), col, s); }

static std::string trunc(const std::string& s, int maxW, int scale = 2)
{
    int cw = (5 + 1) * scale;
    int max = maxW / cw;
    if ((int)s.size() <= max) return s;
    if (max < 3) return s.substr(0, max);
    return s.substr(0, max - 3) + "...";
}

// ─── Icon (real decoded artwork, falls back to a coloured square + letter) ───
static u32 titleColour(u64 id)
{
    return RGBA8(60 + (u8)((id >> 16) & 0x7F),
                 60 + (u8)((id >>  8) & 0x7F),
                120 + (u8)((id)       & 0x5F), 255);
}

static void drawIcon(int x, int y, const GameEntry& g)
{
    if (g.hasIcon && (int)g.icon.size() == ICON_SIZE * ICON_SIZE) {
        for (int iy = 0; iy < ICON_SIZE; iy++)
            for (int ix = 0; ix < ICON_SIZE; ix++)
                px(x + ix, y + iy, g.icon[(size_t)iy * ICON_SIZE + ix]);
    } else {
        rect(x, y, ICON_SIZE, ICON_SIZE, titleColour(g.titleId));
        char ltr[2] = { g.name.empty() ? '?' : (char)toupper(g.name[0]), '\0' };
        drawStr(x + ICON_SIZE/2 - 5*2, y + ICON_SIZE/2 - 7, ltr, COL_WHITE, 2);
    }

    hline(x, y,              ICON_SIZE, COL_ACCENT);
    hline(x, y+ICON_SIZE-1,  ICON_SIZE, COL_ACCENT);
    vline(x, y,              ICON_SIZE, COL_ACCENT);
    vline(x+ICON_SIZE-1, y,  ICON_SIZE, COL_ACCENT);
}

// ─── Scrollbar ────────────────────────────────────────────────────────────────
static void drawScrollbar(int total, int visible, int offset)
{
    if (total <= visible) return;
    int trackH = SCREEN_H - LIST_TOP - 48;
    int tx     = SCREEN_W - 10;
    int ty     = LIST_TOP;
    rect(tx, ty, 6, trackH, COL_HEADER_BG);
    int th = std::max(20, trackH * visible / total);
    int fy = ty + (trackH - th) * offset / std::max(1, total - visible);
    rect(tx+1, fy, 4, th, COL_ACCENT);
}

// ─── Detail panel ─────────────────────────────────────────────────────────────
static void drawDetail(const GameEntry& g, int scroll, int selIdx)
{
    rect(DETAIL_X - 10, LIST_TOP - 4,
         SCREEN_W - DETAIL_X + 10, SCREEN_H - LIST_TOP + 4, COL_HEADER_BG);
    vline(DETAIL_X - 12, LIST_TOP - 4, SCREEN_H - LIST_TOP + 4, COL_DIVIDER);

    int x = DETAIL_X, y = DETAIL_Y;

    drawStr(x, y, trunc(g.name, DETAIL_W, 2), COL_ACCENT, 2);
    y += 7*2 + 12;

    char buf[64];
    snprintf(buf, sizeof(buf), "Total: %s", formatPlaytime(g.totalSeconds).c_str());
    drawStr(x, y, buf, COL_HOURS, 2);
    y += 7*2 + 16;

    hline(x, y, DETAIL_W - 20, COL_DIVIDER);
    y += 8;

    if (g.dailyLog.empty()) {
        // Total playtime can be non-zero even with no daily log: the PDM
        // event log is a limited-depth ring buffer, so old sessions can be
        // rotated out while the lifetime stat total is still tracked.
        drawStr(x, y, "Daily breakdown:", COL_TEXT_DIM, 1);
        y += 7 + 8;
        drawStr(x, y, "No recent session log (log window rotated out)", COL_TEXT_DIM, 1);
        return;
    }

    int total = (int)g.dailyLog.size();
    if (selIdx < 0) selIdx = 0;
    if (selIdx >= total) selIdx = total - 1;

    // Big callout for whichever day is currently selected — this is the
    // "how much did I play on that specific day" answer.
    const DailyPlaytime& sel = g.dailyLog[selIdx];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", sel.year, sel.month, sel.day);
    drawStr(x, y, buf, COL_WHITE, 2);
    std::string selPt = formatHoursMinutes(sel.seconds);
    drawStr(x + DETAIL_W - 20 - (int)selPt.size()*12, y, selPt, COL_ACCENT, 2);
    y += 7*2 + 10;

    drawStr(x, y, "Up/Down: pick a day", COL_TEXT_DIM, 1);
    y += 7 + 10;

    hline(x, y, DETAIL_W - 20, COL_DIVIDER);
    y += 8;

    int start = std::min(scroll, std::max(0, total - DAYS_VISIBLE));
    int end   = std::min(start + DAYS_VISIBLE, total);

    for (int i = start; i < end; i++) {
        const DailyPlaytime& d = g.dailyLog[i];
        bool isSel = (i == selIdx);

        if (isSel) {
            rect(x - 6, y - 2, DETAIL_W - 8, 7 + 8, COL_SELECTED_BG);
            rect(x - 6, y - 2, 3, 7 + 8, COL_ACCENT);
        }

        char date[16];
        snprintf(date, sizeof(date), "%04d-%02d-%02d", d.year, d.month, d.day);
        drawStr(x, y, date, isSel ? COL_WHITE : COL_TEXT, 1);
        std::string pt = formatHoursMinutes(d.seconds);
        drawStr(SCREEN_W - 30 - (int)pt.size() * 6, y, pt, COL_HOURS, 1);
        y += 7 + 6;
    }

    if (total > DAYS_VISIBLE) {
        snprintf(buf, sizeof(buf), "%d/%d days", end, total);
        drawStr(x, SCREEN_H - 60, buf, COL_TEXT_DIM, 1);
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

void renderLoading(Framebuffer* fb, float progress)
{
    u32 stride;
    gFb     = (u32*)framebufferBegin(fb, &stride);
    gStride = stride / sizeof(u32);

    rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);

    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    int bw = 400, bh = 8;
    int bx = (SCREEN_W - bw) / 2, by = 360 - bh/2;
    int fill = (int)(progress * bw);
    rect(bx, by, bw, bh, COL_HEADER_BG);
    if (fill > 0) rect(bx, by, fill, bh, COL_ACCENT);

    drawStr((SCREEN_W - 16*6*2)/2, by - 30, "GameTime Tracker", COL_ACCENT, 2);

    char pct[16];
    snprintf(pct, sizeof(pct), "Loading... %d%%", (int)(progress * 100.0f));
    drawStr((SCREEN_W - (int)strlen(pct)*6)/2, by + 20, pct, COL_TEXT_DIM, 1);

    framebufferEnd(fb);
}

void renderFrame(
    Framebuffer*                  fb,
    const std::vector<GameEntry>& games,
    int selectedIndex,
    int scrollOffset,
    bool showDetail,
    const GameEntry*              detailGame,
    int detailScroll,
    int detailSel)
{
    u32 stride;
    gFb     = (u32*)framebufferBegin(fb, &stride);
    gStride = stride / sizeof(u32);

    // Background
    rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);

    // Header
    rect(0, 0, SCREEN_W, HEADER_H, COL_HEADER_BG);
    const char* title = "GameTime Tracker";
    int tw = (int)strlen(title) * 6 * 3;
    drawStr((SCREEN_W - tw) / 2, (HEADER_H - 7*3) / 2, title, COL_ACCENT, 3);
    drawStr(20, HEADER_H/2 - 4, "Installed Games", COL_TEXT_DIM, 1);
    const char* hint = "A:Details  B:Exit";
    drawStr(SCREEN_W - (int)strlen(hint)*6 - 20, HEADER_H/2 - 4, hint, COL_TEXT_DIM, 1);

    // Divider
    for (int t = 0; t < 3; t++) hline(0, DIVIDER_Y + t, SCREEN_W, COL_ACCENT);

    // Game list
    if (games.empty()) {
        drawStr(50, LIST_TOP + 60, "No installed games found.", COL_TEXT_DIM, 2);
    } else {
        int listW = showDetail ? DETAIL_X - 20 : SCREEN_W - 20;

        for (int i = 0; i < VISIBLE_ITEMS; i++) {
            int idx = scrollOffset + i;
            if (idx >= (int)games.size()) break;
            const GameEntry& g = games[idx];
            int iy  = LIST_TOP + i * ITEM_H;
            bool sel = (idx == selectedIndex);

            if (sel) {
                rect(0, iy, listW, ITEM_H - 2, COL_SELECTED_BG);
                rect(0, iy, 4, ITEM_H - 2, COL_ACCENT);
            }

            drawIcon(ICON_MARGIN, iy + (ITEM_H - ICON_SIZE) / 2, g);

            std::string name = trunc(g.name, listW - TEXT_MARGIN_L - 160, 2);
            drawStr(TEXT_MARGIN_L, iy + 10, name, sel ? COL_WHITE : COL_TEXT, 2);

            // Green stat = pure hours/minutes for this game total.
            std::string hrs = formatHoursMinutes(g.totalSeconds);
            drawStr(listW - (int)hrs.size()*12 - 16, iy + 10, hrs, COL_HOURS, 2);

            // Headline stat = total playtime expressed in days, since that's
            // the more meaningful "how much have I sunk into this" number.
            char sub[48];
            if (g.totalSeconds > 0)
                snprintf(sub, sizeof(sub), "%s total", formatDays(g.totalSeconds).c_str());
            else
                snprintf(sub, sizeof(sub), "Not played yet");
            drawStr(TEXT_MARGIN_L, iy + 36, sub, COL_TEXT_DIM, 1);

            hline(ICON_MARGIN, iy + ITEM_H - 2, listW - ICON_MARGIN*2, COL_DIVIDER);
        }

        drawScrollbar((int)games.size(), VISIBLE_ITEMS, scrollOffset);
    }

    if (showDetail && detailGame)
        drawDetail(*detailGame, detailScroll, detailSel);

    // Footer
    rect(0, SCREEN_H - 44, SCREEN_W, 44, COL_HEADER_BG);
    hline(0, SCREEN_H - 44, SCREEN_W, COL_DIVIDER);

    if (showDetail)
        drawStr(20, SCREEN_H - 30, "B:Back  Up/Down or Stick: pick a day", COL_TEXT_DIM, 1);
    else
        drawStr(20, SCREEN_H - 30,
                "Up/Down or Stick: navigate   A: view details   B: exit",
                COL_TEXT_DIM, 1);

    framebufferEnd(fb);
}
