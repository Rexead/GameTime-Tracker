**[English](BUILDING.md)** | [Українська](BUILDING.uk.md) | [Русский](BUILDING.ru.md)

# Building from source

## Prerequisites

You need **devkitPro** with the Switch toolchain (`devkitA64` + `libnx`), plus
**libjpeg-turbo** for the Switch (used to decode game icon JPEGs embedded in
`NsApplicationControlData`).

### Windows

1. Download and run the [devkitPro graphical installer](https://github.com/devkitPro/installer/releases)
   for Windows. When it asks which platforms to install, make sure
   **Switch development** is ticked.
2. Once it finishes, open **Start → devkitPro → MSYS2** (this is a bundled
   MSYS2 shell with the devkitPro package repos already configured — use it
   instead of a regular `cmd`/PowerShell window for all the steps below).
3. In that MSYS2 window, install the Switch toolchain and libjpeg-turbo:
   ```bash
   pacman -S switch-dev switch-libjpeg-turbo
   ```
4. Close and reopen the MSYS2 window once (so the `DEVKITPRO` environment
   variable set by the installer is picked up), then verify it:
   ```bash
   echo $DEVKITPRO
   ```
   This should print something like `/c/devkitPro`. If it's empty, the
   installer didn't finish correctly — rerun it.
5. Continue with the **Build** steps below, run from the same MSYS2 window.

> **Note:** `git` isn't included by default in the devkitPro MSYS2 shell. You
> can either install it there (`pacman -S git`) or just download the
> repository as a ZIP from GitHub (**Code → Download ZIP**) and extract it,
> then `cd` into that folder from MSYS2 instead of using `git clone`.

### Linux / macOS

Install via the [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman)
package manager:
```bash
sudo dkp-pacman -S switch-dev switch-libjpeg-turbo
```
Make sure `DEVKITPRO` is set in your environment (the installer does this for
you; verify with `echo $DEVKITPRO`, log out/in again if it's empty right
after installing).

## Build

```bash
git clone https://github.com/Rexead/GameTime-Tracker.git
cd GameTime-Tracker
make
```

This produces `GameTime-Tracker.nro` in the project root (the output name
comes from the folder name — rename the folder if you want a different
`.nro` name).

`make clean` removes the `build/` directory and all build artifacts.

## Project layout

```
GameTime-Tracker/
├── source/          # .cpp files (compiled by the Makefile)
│   ├── main.cpp         # app loop, input handling, screen state
│   ├── game_loader.cpp  # NS/PDM queries, icon JPEG decoding, background load thread
│   ├── renderer.cpp     # framebuffer drawing (list, detail view, loading bar, bitmap font)
│   └── utils.cpp        # playtime formatting helpers
├── include/
│   └── gametime.h       # shared structs, UI constants, function declarations
└── Makefile
```

## Installing on console

See the [Installation](README.md#installation) section of the README —
the same `.nro` you build here can be dropped onto the SD card the same way
as a Releases build.
