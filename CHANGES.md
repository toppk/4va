# Changes

## Unreleased

### New
- `-rbuffer` (default) draws each frame into an off-screen pixmap and copies
  it to the window in one step, removing flicker, half-drawn frames and
  leftover pixels. `-rdirect` keeps the original draw-to-window rendering.
- `-aa` draws anti-aliased lines at sub-pixel precision with XRender
  (requires `-rbuffer`). `-lw` now accepts fractional widths, which `-aa`
  honors.
- `-rpresent` shows frames through the X Present extension, in sync with the
  display refresh: `-fps0` gets a new frame every refresh, and rates that
  divide the refresh rate hold each frame for whole refreshes. Works with `-aa`.

### Build
- New `xrender` feature option (default `auto`); without it `-aa` falls back
  to ordinary lines with a warning.
- New `xpresent` feature option (default `auto`); without it `-rpresent`
  falls back to `-rbuffer` with a warning.

### Fixes
- The X event queue is now read every frame: resizes come from
  `ConfigureNotify` and exposed areas are repainted, instead of events
  piling up unread.
- Line endpoints are rounded instead of truncated, reducing vertex wobble.
- Lines are clipped before being sent to X, so extreme perspective no longer
  overflows X's 16-bit coordinates and draws stray lines across the window.

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
