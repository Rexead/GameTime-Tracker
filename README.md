[README.md](https://github.com/user-attachments/files/30104042/README.md)
# Switch-playtime
Nintendo Switch homebrew app for tracking playtime across your installed games

# GameTime Tracker

**[English](README.md)** | [Українська](README.uk.md) | [Русский](README.ru.md)

Nintendo Switch homebrew that shows how much time you've spent in your games.

Tested on firmware **20.5.0** — runs stably.

> **The source code for this repository is currently private.** I'm still polishing and cleaning up the project, and will open it up once it's in a good state. The compiled `.nro` is available in Releases.

## ⚠️ Disclaimer

Requires a modded / chipped console.

## Features

1. On launch, the app loads playtime data for all your installed games, then opens a main menu listing every game alongside how much time you've spent in it.
2. On the left of each entry — total days played; on the right — hours played.
3. Select a game and press **A** to open its detail view, where you can scroll through individual days and see exactly how much time you played on each one.

## Controls

**Main menu:**
- D-pad (up/down) or left stick — navigate the game list
- **A** — open game details
- **B** — exit the app

**Game detail menu:**
- D-pad or left stick — scroll through days
- **B** — return to the main menu

## Roadmap

The app will keep getting small updates over time:

- [ ] View playtime for games you've since uninstalled
- [ ] Custom font support
- [ ] Language selection
- [ ] Bug fixes and small improvements as they come up

## Installation

1. Open **DBI**, select **"Start MTP connection"** — this opens your console's storage as a folder on your PC.
2. Navigate to `Switch\1: SD Card\switch`. It's recommended to create a `switch-playtime` folder here and drop the `.nro` file from the Releases section into it.
3. Wait for the transfer to finish, then exit DBI.
4. If your firmware is a bit outdated (like mine), open the Homebrew Menu (e.g. **Sphaira**), select **GameTime Tracker**, press **X** for the options menu, and choose **"Install forwarder"** — an icon will appear right on your Switch home screen.
