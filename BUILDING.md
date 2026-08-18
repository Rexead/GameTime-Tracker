# Building from source

## Prerequisites

1. **devkitPro** with the Switch toolchain (`devkitA64`) and **libnx**.
   Install via the [devkitPro installer/updater](https://github.com/devkitPro/installer),
   or on Linux/macOS with `pacman`:
   ```bash
   sudo dkp-pacman -S switch-dev
   ```
2. **libjpeg-turbo** for the Switch — used to decode game icon JPEGs embedded
   in `NsApplicationControlData`:
   ```bash
   sudo dkp-pacman -S switch-libjpeg-turbo
   ```
3. Make sure `DEVKITPRO` is set in your environment (the devkitPro installer
   does this for you; verify with `echo $DEVKITPRO`).

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
