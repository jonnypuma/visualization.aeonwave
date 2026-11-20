/*
 *  Aeon Fuse Waveform Visualizer for Windows (DirectX 11): the Waveform visualisation of Kodi (visualization.waveform
 *  22.1.1-Piers, "Waveform.vis, a simple visualisation example by MrC"), changed for the Aeon Fuse skin.
 *  What the line is: WaveCore.h (shared with the OpenGL / OpenGL ES version).
 *
 *  Copyright (C) 2008-2026 Team Kodi (https://kodi.tv)
 *  Changes for Aeon Fuse (2026, 1.3.0; 1.4.0: the channel choices of WaveCore.h, a colour per line):
 *  - drawn in pixels inside the viewport Kodi sets for the control: the original added the control's left / top
 *    again and spanned twice its width (only half of a doubled wave landed in the box);
 *  - the line of WaveCore.h (channels, smoothness, curve), of even thickness: a quad per segment;
 *  - the shaders are compiled when the add-on starts (d3dcompiler_47.dll of Windows), so the build needs no
 *    Windows SDK shader compiler.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "WaveCore.h"

#include <windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <kodi/addon-instance/Visualization.h>

#include <cstring>
#include <vector>

namespace
{

const char* const VERTEX_SHADER = R"(
cbuffer cbViewPort : register(b0)
{
  float g_viewPortWidth;
  float g_viewPortHeight;
  float align1;
  float align2;
};
struct VS_OUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};
VS_OUT main(float4 pos : POSITION, float4 col : COLOR)
{
  VS_OUT r = (VS_OUT)0;
  r.pos.x = (pos.x / (g_viewPortWidth / 2.0)) - 1;
  r.pos.y = -(pos.y / (g_viewPortHeight / 2.0)) + 1;
  r.pos.z = pos.z;
  r.pos.w = 1.0;
  r.col = col;
  return r;
}
)";

const char* const PIXEL_SHADER = R"(
float4 main(float4 pos : SV_POSITION, float4 col : COLOR) : SV_TARGET
{
  return col;
}
)";

struct CbViewPort
{
  float width;
  float height;
  float align1;
  float align2;
};

struct Vertex
{
  float x, y, z;
  float col[4];
};

typedef HRESULT(WINAPI* D3DCompileFunc)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
                                        LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

template<class T>
void SafeRelease(T*& p)
{
  if (p)
  {
    p->Release();
    p = nullptr;
  }
}

} // namespace

class ATTR_DLL_LOCAL CVisualizationAeonWave : public kodi::addon::CAddonBase,
                                              public kodi::addon::CInstanceVisualization
{
public:
  CVisualizationAeonWave() = default;
  ~CVisualizationAeonWave() override;

  bool Init() override;
  void Render() override;
  bool IsDirty() override { return true; }
  void AudioData(const float* audioData, size_t audioDataLength) override
  {
    m_core.AudioData(audioData, audioDataLength);
  }

private:
  bool CreateObjects();
  bool Compile(const char* source, const char* target, ID3DBlob** blob);
  void Upload(int count);

  aeonwave::WaveCore m_core;

  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;
  ID3D11VertexShader* m_vShader = nullptr;
  ID3D11PixelShader* m_pShader = nullptr;
  ID3D11InputLayout* m_inputLayout = nullptr;
  ID3D11Buffer* m_vBuffer = nullptr;
  ID3D11Buffer* m_cViewPort = nullptr;
  HMODULE m_compiler = nullptr;
  D3DCompileFunc m_compile = nullptr;

  float m_width = 1.0f;
  float m_height = 1.0f;
  std::vector<Vertex> m_verts;
  bool m_startOK = false;
};

CVisualizationAeonWave::~CVisualizationAeonWave()
{
  SafeRelease(m_cViewPort);
  SafeRelease(m_vBuffer);
  SafeRelease(m_inputLayout);
  SafeRelease(m_vShader);
  SafeRelease(m_pShader);
  SafeRelease(m_device);
  if (m_compiler)
    FreeLibrary(m_compiler);
}

bool CVisualizationAeonWave::Init()
{
  m_width = static_cast<float>(Width() > 0 ? Width() : 1);
  m_height = static_cast<float>(Height() > 0 ? Height() : 1);
  m_context = static_cast<ID3D11DeviceContext*>(Device());
  if (!m_context)
    return false;
  m_context->GetDevice(&m_device);
  m_core.ReadSettings();
  m_verts.resize(aeonwave::MAX_CURVE * 6 + 6);
  if (!CreateObjects())
  {
    kodi::Log(ADDON_LOG_ERROR, "Aeon Fuse Waveform Visualizer: could not create the DirectX objects");
    return false;
  }
  m_startOK = true;
  return true;
}

bool CVisualizationAeonWave::Compile(const char* source, const char* target, ID3DBlob** blob)
{
  ID3DBlob* errors = nullptr;
  HRESULT hr = m_compile(source, strlen(source), nullptr, nullptr, nullptr, "main", target, 0, 0, blob, &errors);
  if (FAILED(hr))
  {
    if (errors)
      kodi::Log(ADDON_LOG_ERROR, "Aeon Fuse Waveform Visualizer: shader: %s", static_cast<const char*>(errors->GetBufferPointer()));
    SafeRelease(errors);
    return false;
  }
  SafeRelease(errors);
  return true;
}

bool CVisualizationAeonWave::CreateObjects()
{
  m_compiler = LoadLibraryW(L"d3dcompiler_47.dll");
  if (!m_compiler)
  {
    kodi::Log(ADDON_LOG_ERROR, "Aeon Fuse Waveform Visualizer: d3dcompiler_47.dll not found");
    return false;
  }
  m_compile = reinterpret_cast<D3DCompileFunc>(GetProcAddress(m_compiler, "D3DCompile"));
  if (!m_compile)
    return false;

  ID3DBlob* vs = nullptr;
  ID3DBlob* ps = nullptr;
  if (!Compile(VERTEX_SHADER, "vs_4_0_level_9_1", &vs) || !Compile(PIXEL_SHADER, "ps_4_0_level_9_1", &ps))
  {
    SafeRelease(vs);
    SafeRelease(ps);
    return false;
  }

  bool ok = SUCCEEDED(m_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &m_vShader));
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  ok = ok && SUCCEEDED(m_device->CreateInputLayout(layout, 2, vs->GetBufferPointer(), vs->GetBufferSize(),
                                                   &m_inputLayout));
  ok = ok && SUCCEEDED(m_device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &m_pShader));
  SafeRelease(vs);
  SafeRelease(ps);
  if (!ok)
    return false;

  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * m_verts.size());
  desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  if (FAILED(m_device->CreateBuffer(&desc, nullptr, &m_vBuffer)))
    return false;

  D3D11_BUFFER_DESC cdesc = {};
  cdesc.ByteWidth = sizeof(CbViewPort);
  cdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cdesc.Usage = D3D11_USAGE_DEFAULT;
  CbViewPort viewPort = {m_width, m_height, 0.0f, 0.0f};
  D3D11_SUBRESOURCE_DATA init = {};
  init.pSysMem = &viewPort;
  return SUCCEEDED(m_device->CreateBuffer(&cdesc, &init, &m_cViewPort));
}

void CVisualizationAeonWave::Upload(int count)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == m_context->Map(m_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, m_verts.data(), sizeof(Vertex) * count);
    m_context->Unmap(m_vBuffer, 0);
  }
}

void CVisualizationAeonWave::Render()
{
  if (!m_startOK)
    return;

  UINT stride = sizeof(Vertex), offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vBuffer, &stride, &offset);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->VSSetShader(m_vShader, nullptr, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_cViewPort);
  m_context->PSSetShader(m_pShader, nullptr, 0);

  const float w = m_width, h = m_height;
  auto put = [&](int i, float x, float y, const float* col) {
    m_verts[i].x = x;
    m_verts[i].y = y;
    m_verts[i].z = 1.0f;
    memcpy(m_verts[i].col, col, sizeof(float) * 4);
  };

  if (m_core.bgColor[3] != 0.0f)
  {
    // the box filled with the background colour (two triangles)
    put(0, 0, 0, m_core.bgColor);
    put(1, w, 0, m_core.bgColor);
    put(2, 0, h, m_core.bgColor);
    put(3, 0, h, m_core.bgColor);
    put(4, w, 0, m_core.bgColor);
    put(5, w, h, m_core.bgColor);
    Upload(6);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);
  }

  for (const auto& line : m_core.Lines())
  {
    float colour[4];
    m_core.Shaded(line.shade, colour);
    const std::vector<aeonwave::Pt>& curve = m_core.Curve(line.channel, line.centre, line.scale);
    const int n = static_cast<int>(curve.size());
    int count = 0;
    // pixels inside the control's box: x 0..w, y 0..h from the top
    auto px = [&](const aeonwave::Pt& p, float& x, float& y) {
      x = p.x * w;
      y = h / 2.0f - p.y * h / 2.0f;
    };
    if (m_core.thickness > 1)
    {
      const float half = m_core.thickness / 2.0f;
      for (int i = 0; i < n - 1; i++)
      {
        float ax, ay, bx, by;
        px(curve[i], ax, ay);
        px(curve[i + 1], bx, by);
        float dx = bx - ax, dy = by - ay;
        float len = std::sqrt(dx * dx + dy * dy);
        float nx = len > 0.0f ? -dy / len : 0.0f, ny = len > 0.0f ? dx / len : 1.0f;
        nx *= half;
        ny *= half;
        put(count++, ax + nx, ay + ny, colour);
        put(count++, ax - nx, ay - ny, colour);
        put(count++, bx + nx, by + ny, colour);
        put(count++, bx + nx, by + ny, colour);
        put(count++, ax - nx, ay - ny, colour);
        put(count++, bx - nx, by - ny, colour);
      }
      m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
    else
    {
      for (int i = 0; i < n; i++)
      {
        float x, y;
        px(curve[i], x, y);
        put(count++, x, y, colour);
      }
      m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    }
    Upload(count);
    m_context->Draw(count, 0);
  }
}

ADDONCREATOR(CVisualizationAeonWave)
