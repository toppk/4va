# Changes

## 1.22 — 2026-09-13

### New
- `-nd` option to run in the foreground instead of forking into the
  background.
- `-fps<rate>` option to control animation speed. Rotation is applied once
  per frame, so this sets how fast objects tumble.
  - `-fps-1`: unlocked, as fast as the machine allows (the old behavior).
  - `-fps0`: match the refresh rate of the display the window is on, detected
    via XRandR, falling back to 60 (default).
  - `-fpsN`: N frames per second.
- Frames are paced with absolute monotonic deadlines; a late frame resyncs
  instead of bursting to catch up.
- Drawing is flushed to the X server at the end of every frame.

### Build
- Replaced the Makefile with Meson (`meson setup build && meson compile -C build`).
  Requires Meson 1.1 or newer.
- New `xrandr` feature option (default `auto`); without it `-fps0` assumes 60Hz.
- `meson install` installs the programs, the man page and the sample objects
  (to `share/4va`).
- Compiles without warnings on current GCC and Clang: converted K&R function
  definitions to ANSI prototypes, added missing standard headers, and made
  `main` return `int`.

### Layout
- Sources moved to `src/`, sample `.4vd` objects to `data/`, and the man page
  to `doc/4va.1`.

## 1.21 — 1992-08-06

Last release by Matt Welsh.
