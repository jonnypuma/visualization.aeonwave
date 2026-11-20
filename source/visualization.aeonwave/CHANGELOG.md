# Aeon Fuse Waveform Visualizer changelog

`visualization.aeonwave`, built from Kodi's Waveform visualisation 22.1.1 (Piers).

## 1.5.2
- Four add-ons, one per build: `visualization.aeonwave.linux-arm64`, `.linux-armv7`, `.linux-x86_64` and `.windows-x64`,
  each named "Aeon Fuse Waveform Visualizer (<platform>)" with the platform in its summary and description. The Skippy
  Repo serves one zip per id, and the three Linux builds all say `<platform>linux</platform>`, so one id could not
  carry them. `build.sh` makes `dist/visualization.aeonwave.<platform>-<version>.zip`. The library files keep their
  names (`visualization.aeonwave.so.<version>`, `visualization.aeonwave.dll`); Kodi loads them under the new ids
  (checked on CoreELEC 22). The older `visualization.aeonwave` can stay installed beside them.
- Settings translated into the Aeon Fuse skin's 22 languages (`resources/language`). The code is unchanged: the
  1.5.2 packages in `dist/` are the 1.5.1 builds repacked with the language folders (no rebuild).

## 1.5.1
- Icon and fanart.

## 1.5.0
- Renamed: Aeon Fuse Waveform Visualizer (was Aeon Waveform). The add-on id stays `visualization.aeonwave`.

## 1.4.0
- Channels setting: Left (default), Right, Both mixed (averaged into one line), Both overlaid (the right line dimmer,
  under the left), or Both as two lines (left above, right below).

## 1.3.0
- Builds for Linux ARM64, Linux ARM 32-bit, Linux x86-64 and Windows x64. The Windows build is a DirectX 11 port that
  draws inside the box Kodi gives it (the stock DirectX Waveform draws at twice the offset and twice the width).
- `build.sh` builds all four in a Debian bookworm Docker container and zips them into `dist/`.
- The drawing is shared by all platforms (`src/WaveCore.h`).

## 1.2.0
- One Smoothness level, 0 (accurate, as Waveform) to 10 (calmest), instead of separate Style, Smoothing and Calm
  settings.
- Height as a percentage (50-200 %).

## 1.1.0
- Smoothing: each point is the average of the audio it covers, evened out with its neighbours and the frame before,
  with soft peak limiting, drawn as a rounded curve. It can be switched off for the accurate look.

## 1.0.0
- First version, for CoreELEC (Linux ARM64, OpenGL ES): the left channel as one thin, even line through the middle of
  the box, with the line thickness in real pixels. The stock Waveform draws both channels, and its steep strokes come
  out thick on OpenGL ES.
