// GameTime Tracker - Nintendo Switch Homebrew
// devkitPro / libnx

#include "gametime.h"
#include <cstring>
#include <algorithm>

static constexpr float STICK_DEAD  = 0.3f;
static constexpr int   REPEAT_INIT = 24;
static constexpr int   REPEAT_RATE = 6;

struct Repeat {
    bool active = false;
    int  delay  = 0;
    bool tick(bool pressed) {
        if (!pressed) { active = false; delay = 0; return false; }
        if (!active)  { active = true;  delay = REPEAT_INIT; return true; }
        if (--delay <= 0) { delay = REPEAT_RATE; return true; }
        return false;
    }
};

enum class Screen { Loading, List, Detail };

struct App {
    Framebuffer            fb;
    std::vector<GameEntry> games;
    int    sel         = 0;
    int    scroll      = 0;
    int    dscroll     = 0;
    int    dsel        = 0; // index into the focused game's dailyLog, for "which day am I looking at"
    Screen screen      = Screen::Loading;
    bool   loaded      = false;
    bool   loadStarted = false;
    bool   running     = true;
};

static void clampScroll(App& a)
{
    int n = (int)a.games.size();
    if (n == 0) { a.sel = 0; a.scroll = 0; return; }
    if (a.sel < 0)   a.sel = 0;
    if (a.sel >= n)  a.sel = n - 1;
    if (a.sel < a.scroll) a.scroll = a.sel;
    if (a.sel >= a.scroll + VISIBLE_ITEMS) a.scroll = a.sel - VISIBLE_ITEMS + 1;
    if (a.scroll < 0) a.scroll = 0;
    int maxScroll = n - VISIBLE_ITEMS;
    if (maxScroll < 0) maxScroll = 0;
    if (a.scroll > maxScroll) a.scroll = maxScroll;
}

static void handleList(App& a, bool navUp, bool navDn, bool btnA, bool btnB)
{
    if (btnB) { a.running = false; return; }
    if (a.games.empty()) return;
    if (navUp) { a.sel--; clampScroll(a); }
    if (navDn) { a.sel++; clampScroll(a); }
    if (btnA)  { a.dscroll = 0; a.dsel = 0; a.screen = Screen::Detail; }
}

static void handleDetail(App& a, bool navUp, bool navDn, bool btnA, bool btnB)
{
    if (btnB || btnA) { a.screen = Screen::List; return; }
    if (a.games.empty()) return;

    int days = (int)a.games[a.sel].dailyLog.size();
    if (days == 0) return;

    if (navUp) a.dsel--;
    if (navDn) a.dsel++;
    if (a.dsel < 0)        a.dsel = 0;
    if (a.dsel >= days)    a.dsel = days - 1;

    // Keep the scroll window following the selected day.
    if (a.dsel < a.dscroll)                    a.dscroll = a.dsel;
    if (a.dsel >= a.dscroll + DAYS_VISIBLE)     a.dscroll = a.dsel - DAYS_VISIBLE + 1;
    int maxDs = days - DAYS_VISIBLE;
    if (maxDs < 0) maxDs = 0;
    if (a.dscroll < 0)       a.dscroll = 0;
    if (a.dscroll > maxDs)   a.dscroll = maxDs;
}

int main(int argc, char* argv[])
{
    App app;

    framebufferCreate(&app.fb, nwindowGetDefault(),
                      SCREEN_W, SCREEN_H, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&app.fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    Repeat rUp, rDn;

    while (appletMainLoop() && app.running)
    {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);

        HidAnalogStickState stick = padGetStickPos(&pad, 0);
        float sy = (float)stick.y / 32767.0f;

        bool navUp = (down & HidNpadButton_Up)   || (down & HidNpadButton_StickLUp);
        bool navDn = (down & HidNpadButton_Down)  || (down & HidNpadButton_StickLDown);
        if (rUp.tick(sy >  STICK_DEAD)) navUp = true;
        if (rDn.tick(sy < -STICK_DEAD)) navDn = true;

        bool btnA = (down & HidNpadButton_A) != 0;
        bool btnB = (down & HidNpadButton_B) != 0;

        if (!app.loaded) {
            // Kick the scan off once, on a background thread, so the loop
            // keeps rendering frames (and an animated progress bar) instead
            // of blocking on nsGetApplicationControlData() etc.
            if (!app.loadStarted) {
                startGameLoad(&app.games);
                app.loadStarted = true;
            }

            int total = g_loadTotal.load();
            int done  = g_loadProgress.load();
            float progress = total > 0 ? (float)done / (float)total : 0.0f;
            renderLoading(&app.fb, progress);

            if (isGameLoadDone()) {
                finishGameLoad();
                app.screen = Screen::List;
                app.loaded = true;
            }
            continue;
        }

        if (app.screen == Screen::List) {
            handleList(app, navUp, navDn, btnA, btnB);
        } else if (app.screen == Screen::Detail) {
            handleDetail(app, navUp, navDn, btnA, btnB);
        }

        if (!app.running) break;

        if (app.screen == Screen::List) {
            renderFrame(&app.fb, app.games, app.sel, app.scroll,
                        false, nullptr, 0);
        } else if (app.screen == Screen::Detail && !app.games.empty()) {
            renderFrame(&app.fb, app.games, app.sel, app.scroll,
                        true, &app.games[app.sel], app.dscroll, app.dsel);
        }
    }

    // Safety net: if we somehow exit while the background load is still
    // running (shouldn't happen given the loop above), make sure the
    // thread is joined rather than leaked.
    if (app.loadStarted && !isGameLoadDone()) {
        finishGameLoad();
    }

    // Everything below is RAII-owned (App is a stack object), so this isn't
    // strictly required — the vectors free themselves when app goes out of
    // scope, and the whole process's memory is reclaimed by the OS on exit
    // regardless. Cleared explicitly anyway for tidiness / to make it easy
    // to see the icon buffers going away in a heap profiler.
    app.games.clear();
    app.games.shrink_to_fit();

    framebufferClose(&app.fb);
    return 0;
}
