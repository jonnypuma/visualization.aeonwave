/*
 *  Aeon Fuse Waveform Visualizer: the Waveform visualisation of Kodi (visualization.waveform 22.1.1-Piers), changed for the
 *  Aeon Fuse skin's Now playing card. OpenGL / OpenGL ES drawing; what the line is: WaveCore.h.
 *
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2008-2026 Team Kodi (https://kodi.tv)
 *  Changes for Aeon Fuse (2026):
 *  1.0.0  one channel by default, drawn through the middle of the box; a line of even thickness in pixels (the
 *         original's thickness was a share of the box height applied in both directions, so steep strokes grew as
 *         wide as the box is wide); fewer points by default; a quad (two triangles) per segment.
 *  1.1.0  a calmer line: every point is the average of the audio it covers (Waveform takes one raw sample, so
 *         every high note became a spike), then smoothed with its neighbours and with the frame before; large
 *         peaks are softly limited; drawn as a smooth curve (Catmull-Rom) through fewer points.
 *  1.2.0  all of that in degrees: setting Smoothness 0-10 (0 = accurate: one raw sample per point, straight lines,
 *         as Waveform; 10 = the 1.1.0 line); default 5. Height is a percentage on top.
 *  1.3.0  the line itself in WaveCore.h, shared with the DirectX (Windows) version; builds for Linux ARM64,
 *         ARM 32-bit, x86-64 and Windows x64.
 *  1.4.0  Channels: Left, Right, Both mixed, Both overlaid (one band, right dimmer), Both as two lines.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "WaveCore.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <kodi/addon-instance/Visualization.h>
#include <kodi/gui/gl/GL.h>
#include <kodi/gui/gl/Shader.h>

#include <vector>

class ATTR_DLL_LOCAL CVisualizationAeonWave : public kodi::addon::CAddonBase,
                                              public kodi::addon::CInstanceVisualization,
                                              public kodi::gui::gl::CShaderProgram
{
public:
  CVisualizationAeonWave() = default;
  ~CVisualizationAeonWave() override = default;

  bool Init() override;
  void DeInit() override;
  void Render() override;
  bool IsDirty() override { return true; }
  void AudioData(const float* audioData, size_t audioDataLength) override
  {
    m_core.AudioData(audioData, audioDataLength);
  }

  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  void DrawLine(const std::vector<aeonwave::Pt>& curve);

  aeonwave::WaveCore m_core;
  glm::mat4 m_modelProjMat;

#ifdef HAS_GL
  GLuint m_vertexVBO = 0;
#endif
  std::vector<glm::vec3> m_position; // Position x, y, z

  GLint m_uModelProjMatrix = -1;
  GLint m_uColor = -1;
  GLint m_aPosition = -1;

  bool m_startOK = false;
};

bool CVisualizationAeonWave::Init()
{
  std::string fraqShader =
      kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl");
  std::string vertShader =
      kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/vert.glsl");
  if (!LoadShaderFiles(vertShader, fraqShader) || !CompileAndLink())
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to create or compile shader");
    return false;
  }

  m_core.ReadSettings();
  if (m_core.thickness == 1)
    glLineWidth(1.0f);
  m_position.resize(aeonwave::MAX_CURVE * 6);

#ifdef HAS_GL
  glGenBuffers(1, &m_vertexVBO);
#endif

  m_modelProjMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));

  m_startOK = true;
  return true;
}

void CVisualizationAeonWave::DeInit()
{
  if (!m_startOK)
    return;

  m_startOK = false;

#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vertexVBO);
  m_vertexVBO = 0;
#endif
}

void CVisualizationAeonWave::Render()
{
  if (!m_startOK)
    return;

#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        BUFFER_OFFSET(offsetof(glm::vec3, x)));
  glEnableVertexAttribArray(m_aPosition);

  glEnable(GL_LINE_SMOOTH);
#else
  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, 0, m_position.data());
  glEnableVertexAttribArray(m_aPosition);
#endif

  if (m_core.bgColor[3] != 0.0f)
  {
    glClearColor(m_core.bgColor[0], m_core.bgColor[1], m_core.bgColor[2], m_core.bgColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glDisable(GL_BLEND);

  EnableShader();

  for (const auto& line : m_core.Lines())
  {
    float colour[4];
    m_core.Shaded(line.shade, colour);
    glUniform4fv(m_uColor, 1, colour);
    DrawLine(m_core.Curve(line.channel, line.centre, line.scale));
  }

  DisableShader();

  glDisableVertexAttribArray(m_aPosition);

  glEnable(GL_BLEND);

#ifdef HAS_GL
  glDisable(GL_LINE_SMOOTH);
#endif
}

void CVisualizationAeonWave::DrawLine(const std::vector<aeonwave::Pt>& curve)
{
  int mode;
  int ptr = 0;
  const int n = static_cast<int>(curve.size());
  auto ndc = [](const aeonwave::Pt& p) { return glm::vec2(-1.0f + 2.0f * p.x, p.y); };

  if (m_core.thickness > 1)
  {
    // a quad (two triangles) per segment, its sides `thickness` pixels apart: the normal is worked out in pixels,
    // then turned back into the -1..1 units of the box, so flat and steep strokes are equally thin
    const float w = static_cast<float>(Width() > 0 ? Width() : 1);
    const float h = static_cast<float>(Height() > 0 ? Height() : 1);
    const float half = static_cast<float>(m_core.thickness) / 2.0f;
    for (int i = 0; i < n - 1; i++)
    {
      const glm::vec2 A = ndc(curve[i]);
      const glm::vec2 B = ndc(curve[i + 1]);
      glm::vec2 d((B.x - A.x) * w / 2.0f, (B.y - A.y) * h / 2.0f); // segment in pixels
      float len = glm::length(d);
      glm::vec2 nrm = len > 0.0f ? glm::vec2(-d.y, d.x) / len : glm::vec2(0.0f, 1.0f);
      glm::vec2 off(nrm.x * half * 2.0f / w, nrm.y * half * 2.0f / h); // back to -1..1 units

      m_position[ptr++] = glm::vec3(A + off, 1.0f);
      m_position[ptr++] = glm::vec3(A - off, 1.0f);
      m_position[ptr++] = glm::vec3(B + off, 1.0f);
      m_position[ptr++] = glm::vec3(B + off, 1.0f);
      m_position[ptr++] = glm::vec3(A - off, 1.0f);
      m_position[ptr++] = glm::vec3(B - off, 1.0f);
    }
    mode = GL_TRIANGLES;
  }
  else
  {
    for (int i = 0; i < n; i++)
      m_position[ptr++] = glm::vec3(ndc(curve[i]), 1.0f);
    mode = GL_LINE_STRIP;
  }

#ifdef HAS_GL
  glBufferData(GL_ARRAY_BUFFER, ptr * sizeof(glm::vec3), m_position.data(), GL_STATIC_DRAW);
#endif

  glDrawArrays(mode, 0, ptr);
}

void CVisualizationAeonWave::OnCompiledAndLinked()
{
  m_uModelProjMatrix = glGetUniformLocation(ProgramHandle(), "u_modelViewProjectionMatrix");
  m_uColor = glGetUniformLocation(ProgramHandle(), "u_color");

  m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
}

bool CVisualizationAeonWave::OnEnabled()
{
  glUniformMatrix4fv(m_uModelProjMatrix, 1, GL_FALSE, glm::value_ptr(m_modelProjMat));
  glUniform4fv(m_uColor, 1, m_core.lineColor);

  return true;
}

ADDONCREATOR(CVisualizationAeonWave)
