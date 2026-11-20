<div align="center">
  <img src="https://jonnypuma.github.io/assets/visualization.aeonwave/icon.webp" width="140" alt="Aeon Fuse Waveform Visualizer">
  <h1>Aeon Fuse Waveform Visualizer</h1>
  <p><b>One thin waveform line, made for the Aeon Fuse Now playing card.</b></p>
  <p><a href="https://jonnypuma.github.io/#/visualization.aeonwave"><b>Project page</b></a> · <a href="https://jonnypuma.github.io/"><b>All add-ons and apps by jonnypuma</b></a> · <a href="https://jonnypuma.github.io/#install"><b>Install from the Skippy Repo</b></a> · <a href="source/README.md"><b>Source and build script</b></a></p>
</div>

<p align="center"><sub>Version 1.5.2 · Kodi 22 (Piers) · add-on ids <code>visualization.aeonwave.&lt;platform&gt;</code></sub></p>

A calm, thin audio waveform for the Aeon Fuse Now Playing screen. It is Kodi's own Waveform visualisation (by MrC and
Team Kodi) reworked to sit cleanly in the skin's waveform box instead of filling it with spikes.

## Screenshot

The thin waveform on the Now playing card while a song plays. Click for the full size.

<table>
  <tr>
    <td width="100%" align="center" valign="top"><a href="https://jonnypuma.github.io/assets/skin.aeon.fuse/aeon/music_lyrics.webp"><img src="https://jonnypuma.github.io/assets/skin.aeon.fuse/aeon/t/music_lyrics.webp" alt="Aeon Fuse Now playing card with the waveform (The (International) Noise Conspiracy, Smash It Up)"></a><br><sub>Aeon Fuse Now playing card with the waveform (The (International) Noise Conspiracy, Smash It Up)</sub></td>
  </tr>
</table>
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

One add-on per platform, because Kodi cannot tell the three Linux builds apart. Install the one for your device.

| Add-on | For |
|---|---|
| `visualization.aeonwave.linux-arm64` | 64-bit ARM, OpenGL ES: CoreELEC / LibreELEC 64-bit, Raspberry Pi 4/5 |
| `visualization.aeonwave.linux-armv7` | 32-bit ARM, OpenGL ES: 32-bit LibreELEC / CoreELEC images |
| `visualization.aeonwave.linux-x86_64` | x86-64 Linux, OpenGL: desktop Linux, LibreELEC Generic |
| `visualization.aeonwave.windows-x64` | Kodi for Windows, DirectX 11 (it also fixes the stock Waveform drawing at twice the offset and width there) |

Not for Android, macOS or iOS.

## Install

From the Skippy Repo: Add-ons > Install from repository > Skippy Repo > Visualisations, and install the build for your device. Or use
Add-ons > Install from zip file (allow unknown sources) with the zip for your platform. Enable it, choose it in
Settings > Player > Music > Visualisation, and open Now Playing again.

The Aeon Fuse skin draws it in its own box on the Now Playing card; other skins show it full screen like any
visualisation.

## Source code

The source, the add-on template and the build script for all four platforms are in [source/](source/README.md).
The finished zips for every platform are in the Skippy Repo.

## Credits and licence

Based on Kodi's Waveform visualisation (visualization.waveform 22.1.1, by MrC and Team Kodi). GPL-2.0-or-later, see
LICENSE.md. Changes: see CHANGELOG.md.
