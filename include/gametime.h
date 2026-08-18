#pragma once

#include <switch.h>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
// Structures
// ─────────────────────────────────────────────────────────────────────────────

struct DailyPlaytime {
    int year;
    int month;
    int day;
    u64 seconds;
};

struct GameEntry {
    u64         titleId;
    std::string name;
    u64         totalSeconds;
    std::vector<DailyPlaytime> dailyLog;
    bool        hasIcon;
    std::vector<u32> icon; // decoded RGBA8 pixels, ICON_SIZE x ICON_SIZE, only valid if hasIcon
};

// ─────────────────────────────────────────────────────────────────────────────
// UI constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int SCREEN_W        = 1280;
static constexpr int SCREEN_H        = 720;

static constexpr u32 COL_BG          = RGBA8(15,  15,  25,  255);
static constexpr u32 COL_HEADER_BG   = RGBA8(25,  25,  45,  255);
static constexpr u32 COL_ACCENT      = RGBA8(90,  160, 255, 255);
static constexpr u32 COL_TEXT        = RGBA8(230, 230, 240, 255);
static constexpr u32 COL_TEXT_DIM    = RGBA8(140, 140, 160, 255);
static constexpr u32 COL_SELECTED_BG = RGBA8(40,  60,  120, 255);
static constexpr u32 COL_DIVIDER     = RGBA8(60,  60,  100, 255);
static constexpr u32 COL_WHITE       = RGBA8(255, 255, 255, 255);
static constexpr u32 COL_HOURS       = RGBA8(100, 220, 140, 255);

static constexpr int HEADER_H        = 80;
static constexpr int DIVIDER_Y       = HEADER_H;
static constexpr int LIST_TOP        = HEADER_H + 4;
static constexpr int ITEM_H          = 72;
static constexpr int ICON_SIZE       = 56;
static constexpr int ICON_MARGIN     = 16;
static constexpr int TEXT_MARGIN_L   = ICON_MARGIN + ICON_SIZE + 14;
static constexpr int VISIBLE_ITEMS   = (SCREEN_H - LIST_TOP - 48) / ITEM_H;

static constexpr int DETAIL_X        = 640;
static constexpr int DETAIL_Y        = LIST_TOP + 10;
static constexpr int DETAIL_W        = SCREEN_W - DETAIL_X - 20;
static constexpr int DAYS_VISIBLE    = 8;

// ─────────────────────────────────────────────────────────────────────────────
// Loading progress (updated by the background load thread, read by the UI)
// ─────────────────────────────────────────────────────────────────────────────

extern std::atomic<int> g_loadProgress; // games processed so far
extern std::atomic<int> g_loadTotal;    // total games to process (0 until known)

// ─────────────────────────────────────────────────────────────────────────────
// Functions
// ─────────────────────────────────────────────────────────────────────────────

// Synchronous loader (does all the work on the calling thread).
bool loadInstalledGames(std::vector<GameEntry>& out);

// Kicks off loadInstalledGames() on a background thread so the UI can keep
// drawing an animated progress bar. Poll isGameLoadDone(), then call
// finishGameLoad() once to join/clean up the thread.
void startGameLoad(std::vector<GameEntry>* out);
bool isGameLoadDone();
void finishGameLoad();

std::string formatPlaytime(u64 seconds);      // combined "1d 11h" style, for single summary labels
std::string formatHoursMinutes(u64 seconds);  // pure "35h 6m", for the green stat and per-day totals
std::string formatDays(u64 seconds);          // pure "1 day" / "3 days", for the headline days-played stat

void renderFrame(
    Framebuffer*                fb,
    const std::vector<GameEntry>& games,
    int selectedIndex,
    int scrollOffset,
    bool showDetail,
    const GameEntry* detailGame,
    int detailScroll,
    int detailSel = -1 // absolute index into detailGame->dailyLog that's currently highlighted
);

// progress: 0.0f..1.0f, drives the fill of the loading bar.
void renderLoading(Framebuffer* fb, float progress);
