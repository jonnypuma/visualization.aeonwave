# Aeon Fuse Waveform Visualizer

`visualization.aeonwave` 1.5.2 · Kodi 22 (Piers)

A calm, thin audio waveform for the Aeon Fuse Now Playing screen. It is Kodi's own Waveform visualisation (by MrC and
Team Kodi) reworked to sit cleanly in the skin's waveform box instead of filling it with spikes.

## What it does

- **One clean line.** A single line through the middle of its box, the same thin width on flat and steep passages
  (the stock Waveform turns thick and blocky on loud music on CoreELEC / LibreELEC boxes).
- **Smooth, not spiky.** Each point averages the audio it covers, is blended with its neighbours and the frame before,
  softly limited at the peaks and drawn as a rounded curve, so high notes do not shoot up as needles.
- **Smoothness 0-10:** 0 is the raw samples, exactly as Waveform draws them; 10 is the calmest.
- **Channels:** Left (default), Right, Both mixed into one line, Both overlaid (the right one dimmer, under the left),
  or Both as two lines (left above, right below, as Waveform).

## Settings

Channels, Smoothness, Height (50-200 %), Points per line, Smooth curve, Line thickness (in real pixels) and colours.
The settings are translated into the Aeon Fuse skin's 22 languages.

## Platforms

Each build is its own add-on, `visualization.aeonwave.<platform>`:

| Package | For |
|---|---|
| linux-arm64 | 64-bit ARM, OpenGL ES: CoreELEC / LibreELEC 64-bit, Raspberry Pi 4/5 |
| linux-armv7 | 32-bit ARM, OpenGL ES: 32-bit LibreELEC / CoreELEC images |
| linux-x86_64 | x86-64 Linux, OpenGL: desktop Linux, LibreELEC Generic |
| windows-x64 | Kodi for Windows, DirectX 11 (it also fixes the stock Waveform drawing at twice the offset and width there) |

Not for Android, macOS or iOS.

## Install

Add-ons > Install from zip file (allow unknown sources) with the zip for your platform, or unpack it into the add-ons
folder. Enable it, choose it in Settings > Player > Music > Visualisation, and open Now Playing again.

The Aeon Fuse skin draws it in its own box on the Now Playing card; other skins show it full screen like any
visualisation.

## Credits and licence

Based on Kodi's Waveform visualisation (visualization.waveform 22.1.1, by MrC and Team Kodi). GPL-2.0-or-later, see
LICENSE.md. Changes: see CHANGELOG.md.
