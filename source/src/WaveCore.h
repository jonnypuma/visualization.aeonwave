/*
 *  Aeon Fuse Waveform Visualizer: what the line is, shared by the OpenGL / OpenGL ES (Main_gl.cpp) and DirectX (Main_dx.cpp)
 *  drawing code: the settings, the audio turned into points, and the curve through them.
 *
 *  Based on the Waveform visualisation of Kodi (visualization.waveform 22.1.1-Piers).
 *  Copyright (C) 2008-2026 Team Kodi (https://kodi.tv), changes for Aeon Fuse (2026).
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace aeonwave
{

constexpr int MAX_POINTS = 512;  // points per line (the averaged samples)
constexpr int MAX_CURVE = 2048;  // points of the drawn curve

// a point of the curve: x 0..1 from left to right, y -1..1 from the bottom to the top of the box
struct Pt
{
  float x;
  float y;
};

class WaveCore
{
public:
  // Channels (setting values): 0 left, 3 right, 2 both mixed into one line, 4 both overlaid in one band (right
  // dimmer, under the left), 1 both as two lines (left above, right below, as Waveform)
  int channels = 0;
  bool accurate = false;  // Smoothness 0: one raw sample per point, no smoothing, calm, limit or curve
  int points = 128;       // points per line
  float span = 0.5f;      // share of its stretch of audio each point averages (1 = all of it)
  int smoothing = 1;      // passes of a 1-2-1 filter over neighbouring points
  float calm = 0.3f;      // share of the previous frame kept (0 = none, 0.9 = very slow)
  float height = 1.3f;    // gain before the soft limit (tanh): a quiet line still moves, peaks stay inside the box
  bool curveOn = true;    // a smooth curve through the points, or straight segments
  int thickness = 2;      // line width in pixels
  float lineColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};
  float bgColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  void ReadSettings()
  {
    kodi::addon::CheckSettingInt("channels", channels);
    kodi::addon::CheckSettingInt("points-per-line", points);
    points = std::max(16, std::min(MAX_POINTS, points));
    // Smoothness 0-10: one setting for how much of each step of AudioData is used
    int level = 5;
    kodi::addon::CheckSettingInt("smoothness", level);
    level = std::max(0, std::min(10, level));
    accurate = level == 0;
    span = level / 10.0f;   // 10: each point averages all the audio it covers
    smoothing = level / 3;  // 10: three 1-2-1 passes
    calm = 0.06f * level;   // 10: 0.6 of the frame before kept
    int heightPct = 100;
    kodi::addon::CheckSettingInt("height", heightPct);
    heightPct = std::max(50, std::min(200, heightPct));
    // averaging takes height away, so the level adds some back (10: x1.6); Height is a percentage on top
    height = (1.0f + 0.06f * level) * heightPct / 100.0f;
    kodi::addon::CheckSettingBoolean("curve", curveOn);
    kodi::addon::CheckSettingInt("line-thickness", thickness);
    thickness = std::max(1, std::min(10, thickness));

    kodi::addon::CheckSettingFloat("line-red", lineColor[0]);
    kodi::addon::CheckSettingFloat("line-green", lineColor[1]);
    kodi::addon::CheckSettingFloat("line-blue", lineColor[2]);
    kodi::addon::CheckSettingFloat("bg-red", bgColor[0]);
    kodi::addon::CheckSettingFloat("bg-green", bgColor[1]);
    kodi::addon::CheckSettingFloat("bg-blue", bgColor[2]);
    bgColor[3] = (bgColor[0] != 0.0f || bgColor[1] != 0.0f || bgColor[2] != 0.0f) ? 1.0f : 0.0f;
  }

  void AudioData(const float* data, size_t length)
  {
    if (!data || length < 2)
      return;

    const size_t frames = length / 2; // interleaved left / right

    if (accurate)
    {
      // as Waveform: one raw sample per point, nothing evened out
      for (int p = 0; p < points; p++)
      {
        const size_t f = std::min(frames - 1, frames * p / points);
        const float l = data[2 * f];
        const float r = data[2 * f + 1];
        m_wave[0][p] = Clamp(l);
        m_wave[1][p] = Clamp(r);
        m_wave[2][p] = Clamp((l + r) * 0.5f);
      }
      return;
    }

    for (int c = 0; c < 3; c++)
    {
      // 1. every point: the average of (a share of) the frames it covers, around its middle (a low-pass; one raw
      //    sample per point made the spikes)
      for (int p = 0; p < points; p++)
      {
        const float mid = frames * (p + 0.5f) / points;
        const float half = frames * span / points / 2.0f;
        size_t from = static_cast<size_t>(std::max(0.0f, mid - half));
        size_t to = std::max(from + 1, static_cast<size_t>(mid + half + 0.5f));
        if (to > frames)
          to = frames;
        if (from >= to)
          from = to - 1;
        float sum = 0.0f;
        for (size_t f = from; f < to; f++)
        {
          const float l = data[2 * f];
          const float r = data[2 * f + 1];
          sum += c == 0 ? l : c == 1 ? r : (l + r) * 0.5f;
        }
        m_work[p] = sum / float(to - from);
      }

      // 2. with the neighbours (1-2-1), a few passes
      for (int pass = 0; pass < smoothing; pass++)
      {
        float prev = m_work[0];
        for (int p = 1; p < points - 1; p++)
        {
          const float cur = m_work[p];
          m_work[p] = (prev + 2.0f * cur + m_work[p + 1]) * 0.25f;
          prev = cur;
        }
      }

      // 3. a soft limit for the peaks, then eased from the frame before
      for (int p = 0; p < points; p++)
      {
        const float target = std::tanh(m_work[p] * height);
        m_wave[c][p] = m_wave[c][p] * calm + target * (1.0f - calm);
      }
    }
  }

  // the line of channel c (0 left, 1 right, 2 mid) around centre (-1..1) swinging by scale, as a curve
  const std::vector<Pt>& Curve(int c, float centre, float scale)
  {
    const float* wave = m_wave[std::max(0, std::min(2, c))];
    m_curve.clear();
    auto at = [&](int i) {
      i = std::max(0, std::min(points - 1, i));
      return Pt{i / float(points - 1), centre + wave[i] * scale};
    };
    const int sub = curveOn && !accurate
                        ? std::max(1, std::min(8, (MAX_CURVE - 1) / std::max(1, points - 1)))
                        : 1;
    for (int i = 0; i < points - 1; i++)
    {
      const Pt p0 = at(i - 1), p1 = at(i), p2 = at(i + 1), p3 = at(i + 2);
      for (int s = 0; s < sub; s++)
      {
        const float t = s / float(sub), t2 = t * t, t3 = t2 * t;
        // Catmull-Rom: passes through every point
        auto cr = [&](float a, float b, float cc, float d) {
          return 0.5f * ((2.0f * b) + (-a + cc) * t + (2.0f * a - 5.0f * b + 4.0f * cc - d) * t2 +
                         (-a + 3.0f * b - 3.0f * cc + d) * t3);
        };
        m_curve.push_back(Pt{cr(p0.x, p1.x, p2.x, p3.x), cr(p0.y, p1.y, p2.y, p3.y)});
      }
    }
    m_curve.push_back(at(points - 1));
    return m_curve;
  }

  // the lines to draw for the Channels setting, in drawing order: {channel, centre, scale, shade}
  // (shade: share of the line colour; the right channel of Both overlaid is dimmer and drawn first, under the left)
  struct Line
  {
    int channel;
    float centre;
    float scale;
    float shade;
  };
  std::vector<Line> Lines() const
  {
    switch (channels)
    {
      case 1:
        return {{0, 0.5f, 0.45f, 1.0f}, {1, -0.5f, 0.45f, 1.0f}};  // two lines: left above, right below
      case 2:
        return {{2, 0.0f, 0.9f, 1.0f}};                           // both mixed into one line
      case 3:
        return {{1, 0.0f, 0.9f, 1.0f}};                           // right
      case 4:
        return {{1, 0.0f, 0.9f, 0.55f}, {0, 0.0f, 0.9f, 1.0f}};   // both overlaid in one band
      default:
        return {{0, 0.0f, 0.9f, 1.0f}};                           // left
    }
  }

  // the line colour for a shade (alpha kept)
  void Shaded(float shade, float out[4]) const
  {
    for (int i = 0; i < 3; i++)
      out[i] = lineColor[i] * shade;
    out[3] = lineColor[3];
  }

private:
  static float Clamp(float v) { return std::max(-1.0f, std::min(1.0f, v)); }

  float m_wave[3][MAX_POINTS] = {};
  float m_work[MAX_POINTS] = {};
  std::vector<Pt> m_curve;
};

} // namespace aeonwave
