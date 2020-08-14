//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/log.h"
#include "core/path_utils.h"
#include "window/window.h"

#include <math.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#define DX_CHECKF_RETURN(condition) NJ_CHECKF_RETURN(condition == S_OK, "")
#define DX_CHECKF_RETURN_FALSE(condition) NJ_CHECKF_RETURN_VAL(condition == S_OK, false, "")

struct nj_dx12_window_t : public nj_window_t {
  nj_dx12_window_t(nj_allocator_t* allocator, const nj_os_char* title, int width, int height)
      : nj_window_t(allocator, title, width, height) {}
  bool init();
  void destroy() override;
  void loop() override;

  IDXGISwapChain3* sc;
  ID3D12Device* device;
  ID3D12Resource* render_targets[2];
  ID3D12CommandAllocator* cmd_allocators[2];
  ID3D12CommandQueue* queue;
  ID3D12RootSignature* root_sig;
  ID3D12DescriptorHeap* rtv_heap;
  ID3D12PipelineState* pso;
  ID3D12GraphicsCommandList* cmd_list;
  int rtv_descriptor_sz;
  ID3D12Resource* vertex_buffer;
  D3D12_VERTEX_BUFFER_VIEW vb_view;
  int frame_i;
  ID3D12Fence* fence;
  uint64_t fence_vals[2];
  HANDLE fence_event;
};

bool nj_dx12_window_t::init() {
  nj_window_t::init();
  UINT dxgi_factory_flags = 0;
  ID3D12Debug* debug_controller;
  DX_CHECKF_RETURN_FALSE(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
  debug_controller->EnableDebugLayer();
  dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

  IDXGIFactory4* factory;
  DX_CHECKF_RETURN_FALSE(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory)));

  IDXGIAdapter1* adapter;
  for (UINT adapter_i = 0; factory->EnumAdapters1(adapter_i, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_i) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      continue;
    if (D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)) == S_OK)
      break;
  }
  NJ_CHECKF_RETURN_VAL(device, false, "Can't find D3D12 compitable graphics card");

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  DX_CHECKF_RETURN_FALSE(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)));

  DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
  sc_desc.BufferCount = 2;
  sc_desc.Width = width;
  sc_desc.Height = height;
  sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sc_desc.SampleDesc.Count = 1;

  IDXGISwapChain1* swap_chain;
  DX_CHECKF_RETURN_FALSE(factory->CreateSwapChainForHwnd(queue, *(HWND*)handle, &sc_desc, NULL, NULL, &swap_chain));
  DX_CHECKF_RETURN_FALSE(factory->MakeWindowAssociation(*(HWND*)handle, DXGI_MWA_NO_ALT_ENTER));

  DX_CHECKF_RETURN_FALSE(swap_chain->QueryInterface(IID_PPV_ARGS(&sc)));
  frame_i = sc->GetCurrentBackBufferIndex();

  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
  rtv_heap_desc.NumDescriptors = 2;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  DX_CHECKF_RETURN_FALSE(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)));
  rtv_descriptor_sz = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (int rt_i = 0; rt_i < 2; ++rt_i) {
    DX_CHECKF_RETURN_FALSE(sc->GetBuffer(rt_i, IID_PPV_ARGS(&render_targets[rt_i])));
    device->CreateRenderTargetView(render_targets[rt_i], NULL, rtv_handle);
    rtv_handle.ptr += rtv_descriptor_sz;
    DX_CHECKF_RETURN_FALSE(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocators[rt_i])));
  }

  D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
  root_sig_desc.NumParameters = 0;
  root_sig_desc.pParameters = NULL;
  root_sig_desc.NumStaticSamplers = 0;
  root_sig_desc.pStaticSamplers = NULL;
  root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ID3DBlob* sig;
  ID3DBlob* error;
  DX_CHECKF_RETURN_FALSE(D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
  DX_CHECKF_RETURN_FALSE(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&root_sig)));

  ID3DBlob* vs;
  ID3DBlob* ps;
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

  nj_os_char shader_path[NJ_MAX_PATH];
  nj_path_from_exe_dir(NJ_OS_LIT("assets/shader.hlsl"), shader_path, NJ_MAX_PATH);
  DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vs, NULL));
  DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &ps, NULL));

  D3D12_INPUT_ELEMENT_DESC input_elem_descs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  D3D12_RASTERIZER_DESC rasterizer_desc = {};
  rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizer_desc.FrontCounterClockwise = FALSE;
  rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizer_desc.DepthClipEnable = TRUE;
  rasterizer_desc.MultisampleEnable = FALSE;
  rasterizer_desc.AntialiasedLineEnable = FALSE;
  rasterizer_desc.ForcedSampleCount = 0;
  rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_BLEND_DESC blend_desc = {};
  blend_desc.AlphaToCoverageEnable = FALSE;
  blend_desc.IndependentBlendEnable = FALSE;

  D3D12_RENDER_TARGET_BLEND_DESC rt_blend_desc = {};
  rt_blend_desc.BlendEnable = FALSE;
  rt_blend_desc.LogicOpEnable = FALSE;
  rt_blend_desc.SrcBlend = D3D12_BLEND_ONE;
  rt_blend_desc.DestBlend = D3D12_BLEND_ZERO;
  rt_blend_desc.BlendOp = D3D12_BLEND_OP_ADD;
  rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_ONE;
  rt_blend_desc.DestBlendAlpha = D3D12_BLEND_ZERO;
  rt_blend_desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  rt_blend_desc.LogicOp = D3D12_LOGIC_OP_NOOP;
  rt_blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  for (int rt_i; rt_i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++rt_i)
    blend_desc.RenderTarget[rt_i] = rt_blend_desc;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.InputLayout = { input_elem_descs, 2};
  pso_desc.pRootSignature = root_sig;
  pso_desc.VS = {};
  pso_desc.VS.pShaderBytecode = vs->GetBufferPointer();
  pso_desc.VS.BytecodeLength = vs->GetBufferSize();
  pso_desc.PS = {};
  pso_desc.PS.pShaderBytecode = ps->GetBufferPointer();
  pso_desc.PS.BytecodeLength = ps->GetBufferSize();
  pso_desc.RasterizerState = rasterizer_desc;
  pso_desc.BlendState = blend_desc;
  pso_desc.DepthStencilState = {};
  pso_desc.DepthStencilState.DepthEnable = FALSE;
  pso_desc.DepthStencilState.StencilEnable = FALSE;
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.SampleDesc.Count = 1;
  DX_CHECKF_RETURN_FALSE(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)));

  DX_CHECKF_RETURN_FALSE(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocators[frame_i], pso, IID_PPV_ARGS(&cmd_list)));

  cmd_list->Close();

  float aspect_ratio = width * 1.0f / height;
  float vertices[] = {
    0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  };

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
  heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heap_props.CreationNodeMask = 1;
  heap_props.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC res_desc = {};
  res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  res_desc.Alignment = 0;
  res_desc.Width = sizeof(vertices);
  res_desc.Height = 1;
  res_desc.DepthOrArraySize = 1;
  res_desc.MipLevels = 1;
  res_desc.Format = DXGI_FORMAT_UNKNOWN;
  res_desc.SampleDesc.Count = 1;
  res_desc.SampleDesc.Quality = 0;
  res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  DX_CHECKF_RETURN_FALSE(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&vertex_buffer)));

  void* vertex_data_begin;
  D3D12_RANGE range = {};
  range.Begin = 0;
  range.End = 0;
  DX_CHECKF_RETURN_FALSE(vertex_buffer->Map(0, &range, &vertex_data_begin));
  memcpy(vertex_data_begin, vertices, sizeof(vertices));
  vertex_buffer->Unmap(0, NULL);

  vb_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
  vb_view.StrideInBytes = sizeof(vertices) / 3;
  vb_view.SizeInBytes = sizeof(vertices);

  DX_CHECKF_RETURN_FALSE(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
  ++fence_vals[frame_i];
  fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  NJ_CHECKF_RETURN_VAL(fence_event, false, "");

  DX_CHECKF_RETURN_FALSE(queue->Signal(fence, fence_vals[frame_i]));

  DX_CHECKF_RETURN_FALSE(fence->SetEventOnCompletion(fence_vals[frame_i], fence_event));
  WaitForSingleObject(fence_event, INFINITE);

  ++fence_vals[frame_i];

  return true;
}

void nj_dx12_window_t::destroy() {
  nj_window_t::destroy();
}

void nj_dx12_window_t::loop() {
  DX_CHECKF_RETURN(cmd_allocators[frame_i]->Reset());
  DX_CHECKF_RETURN(cmd_list->Reset(cmd_allocators[frame_i], pso));

  cmd_list->SetGraphicsRootSignature(root_sig);
  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = width;
  viewport.Height = height;
  viewport.MinDepth = D3D12_MIN_DEPTH;
  viewport.MaxDepth = D3D12_MAX_DEPTH;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0,
  scissor_rect.top = 0,
  scissor_rect.right = width,
  scissor_rect.bottom = height,

  cmd_list->RSSetViewports(1, &viewport);
  cmd_list->RSSetScissorRects(1, &scissor_rect);

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition = {};
  barrier.Transition.pResource = render_targets[frame_i];
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmd_list->ResourceBarrier(1, &barrier);

  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
  rtv_handle.ptr += frame_i * rtv_descriptor_sz;
  cmd_list->OMSetRenderTargets(1, &rtv_handle, FALSE, NULL);

  float clear_color[] = {0.0f, 0.2f, 0.4f, 1.0f};
  cmd_list->ClearRenderTargetView(rtv_handle, clear_color, 0, NULL);
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmd_list->IASetVertexBuffers(0, 1, &vb_view);
  cmd_list->DrawInstanced(3, 1, 0, 0);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  cmd_list->ResourceBarrier(1, &barrier);

  DX_CHECKF_RETURN(cmd_list->Close());

  queue->ExecuteCommandLists(1, (ID3D12CommandList**)&cmd_list);
  DX_CHECKF_RETURN(sc->Present(1, 0));

  const uint64_t current_fence_val = fence_vals[frame_i];
  DX_CHECKF_RETURN(queue->Signal(fence, current_fence_val));

  frame_i = sc->GetCurrentBackBufferIndex();
  if (fence->GetCompletedValue() < fence_vals[frame_i]) {
    DX_CHECKF_RETURN(fence->SetEventOnCompletion(fence_vals[frame_i], fence_event));
    WaitForSingleObject(fence_event, INFINITE);
  }

  fence_vals[frame_i] = current_fence_val + 1;
}

int main(int argc, char** argv) {
  nj_core_init(NJ_OS_LIT("dx12_sample.log"));
  nj_dx12_window_t w(g_general_allocator, NJ_OS_LIT("dx12_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}
