#include "gametime.h"
#include <cstdio>

// Combined "1d 11h" style summary — used for single one-line totals like the
// detail panel header, where showing both units together reads naturally.
std::string formatPlaytime(u64 seconds)
{
    if (seconds == 0) return "0m";

    u64 d = seconds / 86400;
    u64 h = (seconds % 86400) / 3600;
    u64 m = (seconds % 3600) / 60;

    char buf[32];
    if (d > 0)
        snprintf(buf, sizeof(buf), "%llud %lluh",
                 (unsigned long long)d, (unsigned long long)h);
    else if (h > 0)
        snprintf(buf, sizeof(buf), "%lluh %llum",
                 (unsigned long long)h, (unsigned long long)m);
    else
        snprintf(buf, sizeof(buf), "%llum", (unsigned long long)m);

    return std::string(buf);
}

// Pure hours/minutes, no day rollup — e.g. "35h 6m". Used for the green
// stat next to a game and for per-day totals in the detail view.
std::string formatHoursMinutes(u64 seconds)
{
    if (seconds == 0) return "0m";

    u64 h = seconds / 3600;
    u64 m = (seconds % 3600) / 60;

    char buf[32];
    if (h > 0)
        snprintf(buf, sizeof(buf), "%lluh %llum",
                 (unsigned long long)h, (unsigned long long)m);
    else
        snprintf(buf, sizeof(buf), "%llum", (unsigned long long)m);

    return std::string(buf);
}

// Pure day count — e.g. "1 day", "3 days", "< 1 day". Used as the headline
// "played for N days" stat in the game list.
std::string formatDays(u64 seconds)
{
    u64 d = seconds / 86400;
    if (d == 0) return "< 1 day";

    char buf[32];
    snprintf(buf, sizeof(buf), "%llu day%s",
             (unsigned long long)d, d == 1 ? "" : "s");
    return std::string(buf);
}
