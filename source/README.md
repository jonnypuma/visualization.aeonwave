# Source: Aeon Fuse Waveform Visualizer 1.5.2

Kodi's Waveform visualisation (visualization.waveform 22.1.1-Piers, by MrC and Team Kodi, GPL-2.0-or-later,
<https://github.com/xbmc/visualization.waveform>) changed for the Aeon Fuse Now playing card. Kodi 22 (Piers).

| Path | What it is |
|---|---|
| `src/WaveCore.h` | the waveform itself, shared by every platform: channels, smoothing, height, points, thickness |
| `src/Main_gl.cpp` | drawing with OpenGL / OpenGL ES (Linux: CoreELEC, LibreELEC, Raspberry Pi, desktop) |
| `src/Main_dx.cpp` | drawing with DirectX 11 (Windows), shaders compiled at start-up with Windows' `d3dcompiler_47.dll` |
| `visualization.aeonwave/` | the add-on template: addon.xml (the Linux line), settings, the GL / GLES shaders, 23 languages |
| `build.sh` | builds all four platforms and zips them |

## What it changes

The stock add-on draws both channels; on OpenGL ES its "thickness" is a share of the box height applied in both
directions, so steep strokes (loud passages) come out as wide as the box is wide, and every point is one raw sample,
so every high note is a spike. Its DirectX version adds the control's left / top a second time and spans twice the
width, so only half of a doubled wave lands in the box.

This one draws:
- Channels: Left (default) or Right as one line through the middle; Both mixed (left and right averaged into one
  line); Both overlaid (both lines in the same band, the right one dimmer); Both as two lines (left above, right below).
- Smoothness 0-10 (default 5): 0 = accurate (one raw sample per point, straight lines). Higher: each point averages
  more of the audio it covers, is evened out with its neighbours and the moment before, peaks are softly limited and
  it is drawn as a rounded curve. Height 50-200 % on top.
- Line thickness in real pixels on flat and steep strokes (default 2 px), points per line (default 128), colours.

## Build

`build.sh` runs inside `debian:bookworm` and builds one add-on per platform with Debian's cross compilers
(aarch64 / armhf g++ 12, native g++ 12, MinGW-w64):

| Add-on | For |
|---|---|
| `visualization.aeonwave.linux-arm64` | 64-bit ARM Linux, OpenGL ES |
| `visualization.aeonwave.linux-armv7` | 32-bit ARM Linux, OpenGL ES |
| `visualization.aeonwave.linux-x86_64` | x86-64 Linux, OpenGL |
| `visualization.aeonwave.windows-x64` | Windows x64, DirectX 11 |

You need Kodi's add-on header kit next to this folder, matching the Kodi you build for (here kodi.binary.global.main
2.0.4, gui 5.15.1, filesystem 1.1.11, instance.visualization 5.0.0, as in addon.xml's requires):

    git clone --depth 1 --filter=blob:none --sparse https://github.com/xbmc/xbmc.git <headers>
    cd <headers> && git sparse-checkout set xbmc/addons/kodi-dev-kit/include
    docker run --rm -v "<this folder>:/src" -v "<headers>:/src/kodi" debian:bookworm sh /src/build.sh

(From Git Bash on Windows: prefix with `MSYS_NO_PATHCONV=1` and pass the folders through `cygpath -w`.)

Out: `dist/<platform>/visualization.aeonwave.<platform>/` and `dist/visualization.aeonwave.<platform>-1.5.2.zip`.
Linux builds link a stand-in GL library by its soname (`libGLESv2.so.2` / `libGL.so.1`), so the system's library answers
at run time; glibc 2.36 / GLIBCXX 3.4.30 or newer is needed. The Windows DLL is static (only KERNEL32 and msvcrt).

Not for Android (needs an NDK build and packaging), macOS or iOS.

The finished zips are served by the Skippy Repo; the repository root holds the Windows build as an installable folder.
