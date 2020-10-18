//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/gfx/cam.h"
#include "core/linear_allocator.h"
#include "core/loader/obj.h"
#include "core/log.h"
#include "core/math/float.inl"
#include "core/math/mat4.inl"
#include "core/math/transform.inl"
#include "core/path_utils.h"
#include "core/window/window.h"

#include <math.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#define DX_CHECKF_RETURN(condition) NJ_CHECKF_RETURN(condition == S_OK, "")
#define DX_CHECKF_RETURN_FALSE(condition) NJ_CHECKF_RETURN_VAL(condition == S_OK, false, "")

struct dx12_cb_t {
  nj_m4_t mvp;
  nj_v4_t cam;
  nj_v4_t obj_color;
  nj_v4_t light_pos;
  nj_v4_t light_color;
};

struct nj_dx12_window_t : public nj_window_t {
  nj_dx12_window_t(nj_allocator_t* allocator, const nj_os_char* title, int width, int height)
      : nj_window_t(allocator, title, width, height) {}
  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;

  nj_cam_t cam;
  njsp total_num_vertices = 0;
  IDXGISwapChain3* sc;
  ID3D12Device* device;
  ID3D12Resource* render_targets[2];
  ID3D12CommandAllocator* cmd_allocators[2];
  ID3D12CommandQueue* queue;
  ID3D12RootSignature* root_sig;
  ID3D12DescriptorHeap* rtv_heap;
  ID3D12DescriptorHeap* cbv_heap;
  ID3D12DescriptorHeap* dsv_heap;
  ID3D12PipelineState* pso;
  ID3D12GraphicsCommandList* cmd_list;
  int rtv_descriptor_sz;
  ID3D12Resource* vertex_buffer;
  ID3D12Resource* constant_buffer;
  ID3D12Resource* depth_stencil;
  dx12_cb_t cb;
  void* cbv_p;
  D3D12_VERTEX_BUFFER_VIEW vertices_view;
  D3D12_VERTEX_BUFFER_VIEW normals_view;
  int frame_count;
  int frame_i;
  ID3D12Fence* fence;
  uint64_t fence_vals[3];
  HANDLE fence_event;
};

bool nj_dx12_window_t::init() {
  nj_window_t::init();
  nj_cam_init(&cam, this);
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

  this->frame_count = 2;
  DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
  sc_desc.BufferCount = this->frame_count;
  sc_desc.Width = this->width;
  sc_desc.Height = this->height;
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
  rtv_heap_desc.NumDescriptors = this->frame_count;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  DX_CHECKF_RETURN_FALSE(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)));
  rtv_descriptor_sz = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
  cbv_heap_desc.NumDescriptors = this->frame_count;
  cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  DX_CHECKF_RETURN_FALSE(device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&cbv_heap)));

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
  dsv_heap_desc.NumDescriptors = this->frame_count;
  dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  DX_CHECKF_RETURN_FALSE(device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap)));

  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (int rt_i = 0; rt_i < this->frame_count; ++rt_i) {
    DX_CHECKF_RETURN_FALSE(sc->GetBuffer(rt_i, IID_PPV_ARGS(&render_targets[rt_i])));
    device->CreateRenderTargetView(render_targets[rt_i], NULL, rtv_handle);
    rtv_handle.ptr += rtv_descriptor_sz;
    DX_CHECKF_RETURN_FALSE(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_allocators[rt_i])));
  }

  // Create a root signature consisting of a descriptor table with a single CBV
  D3D12_DESCRIPTOR_RANGE1 ranges;
  ranges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  ranges.NumDescriptors = 1;
  ranges.BaseShaderRegister = 0;
  ranges.RegisterSpace = 0;
  ranges.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
  ranges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER1 root_params;
  root_params.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  root_params.DescriptorTable.NumDescriptorRanges = 1;
  root_params.DescriptorTable.pDescriptorRanges = &ranges;
  root_params.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags= 
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
  root_sig_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  root_sig_desc.Desc_1_1.NumParameters = 1;
  root_sig_desc.Desc_1_1.pParameters = &root_params;
  root_sig_desc.Desc_1_1.NumStaticSamplers = 0;
  root_sig_desc.Desc_1_1.pStaticSamplers = NULL;
  root_sig_desc.Desc_1_1.Flags = root_sig_flags;
  ID3DBlob* signature;
  ID3DBlob* error;
  DX_CHECKF_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
  DX_CHECKF_RETURN_FALSE(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_sig)));

  ID3DBlob* vs;
  ID3DBlob* ps;
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

  nj_linear_allocator_t<> temp_allocator("obj_allocator");
  temp_allocator.init();
  const nj_os_char* obj_paths[] = {
    NJ_OS_LIT("assets/cube.obj"),
    NJ_OS_LIT("assets/plane.obj"),
  };
  int obj_count = sizeof(obj_paths) / sizeof(obj_paths[0]);
  nj_dynamic_array_t<nj_obj_t> objs;
  nj_da_init(&objs, &temp_allocator);
  nj_da_reserve(&objs, obj_count);
  njsp total_vertices_size = 0;
  njsp total_normals_size = 0;
  for (int i = 0; i < obj_count; ++i) {
    nj_os_char obj_path[NJ_MAX_PATH];
    nj_path_from_exe_dir(obj_paths[i], obj_path, NJ_MAX_PATH);
    nj_obj_t obj;
    nj_obj_init(&obj, &temp_allocator, obj_path);
    nj_da_append(&objs, obj);
    total_vertices_size += nj_da_len(&obj.vertices) * sizeof(obj.vertices[0]);
    total_normals_size += nj_da_len(&obj.normals) * sizeof(obj.normals[0]);
    total_num_vertices += nj_da_len(&obj.vertices);
  }

  nj_os_char shader_path[NJ_MAX_PATH];
  nj_path_from_exe_dir(NJ_OS_LIT("assets/shader.hlsl"), shader_path, NJ_MAX_PATH);
  DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vs, NULL));
  DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &ps, NULL));

  D3D12_INPUT_ELEMENT_DESC input_elem_descs[] = {
    { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    { "N", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  D3D12_RASTERIZER_DESC rasterizer_desc = {};
  rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizer_desc.FrontCounterClockwise = TRUE;
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

  for (int rt_i = 0; rt_i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++rt_i)
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
  pso_desc.DepthStencilState.DepthEnable = TRUE;
  pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  pso_desc.DepthStencilState.StencilEnable = FALSE;
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  pso_desc.SampleDesc.Count = 1;
  DX_CHECKF_RETURN_FALSE(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)));

  DX_CHECKF_RETURN_FALSE(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocators[frame_i], pso, IID_PPV_ARGS(&cmd_list)));

  cmd_list->Close();

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
  heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heap_props.CreationNodeMask = 1;
  heap_props.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC res_desc = {};
  res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  res_desc.Alignment = 0;
  res_desc.Width = total_vertices_size + total_normals_size;
  res_desc.Height = 1;
  res_desc.DepthOrArraySize = 1;
  res_desc.MipLevels = 1;
  res_desc.Format = DXGI_FORMAT_UNKNOWN;
  res_desc.SampleDesc.Count = 1;
  res_desc.SampleDesc.Quality = 0;
  res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  DX_CHECKF_RETURN_FALSE(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&vertex_buffer)));

  {
    D3D12_RESOURCE_DESC depth_tex_desc = {};
    depth_tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_tex_desc.Alignment = 0;
    depth_tex_desc.Width = this->width;
    depth_tex_desc.Height = this->height;
    depth_tex_desc.DepthOrArraySize = 1;
    depth_tex_desc.MipLevels = 1;
    depth_tex_desc.Format = DXGI_FORMAT_D32_FLOAT;
    depth_tex_desc.SampleDesc.Count = 1;
    depth_tex_desc.SampleDesc.Quality = 0;
    depth_tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = DXGI_FORMAT_D32_FLOAT;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;
    D3D12_HEAP_PROPERTIES dsv_heap_props = {};
    dsv_heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    dsv_heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    dsv_heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    dsv_heap_props.CreationNodeMask = 1;
    dsv_heap_props.VisibleNodeMask = 1;
    DX_CHECKF_RETURN_FALSE(device->CreateCommittedResource(&dsv_heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&depth_stencil)));
    device->CreateDepthStencilView(depth_stencil, NULL, dsv_heap->GetCPUDescriptorHandleForHeapStart());
  }

  nju8* vertex_data_begin;
  D3D12_RANGE range = {};
  range.Begin = 0;
  range.End = 0;
  DX_CHECKF_RETURN_FALSE(vertex_buffer->Map(0, &range, (void**)&vertex_data_begin));
  njsz vertices_offset = 0;
  for (int i = 0; i < obj_count; ++i) {
    njsz obj_total_vertices_size = nj_da_len(&objs[i].vertices) * sizeof(objs[i].vertices[0]);
    memcpy(vertex_data_begin + vertices_offset, &objs[i].vertices[0], obj_total_vertices_size);
    vertices_offset += obj_total_vertices_size;
  }
  njsz normals_offset = 0;
  for (int i = 0; i < obj_count; ++i) {
    njsz obj_total_normals_size = nj_da_len(&objs[i].normals) * sizeof(objs[i].normals[0]);
    memcpy(vertex_data_begin + total_vertices_size + normals_offset, &objs[i].normals[0], obj_total_normals_size);
    normals_offset += obj_total_normals_size;
  }
  vertex_buffer->Unmap(0, NULL);
  temp_allocator.destroy();

  vertices_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
  vertices_view.SizeInBytes = total_vertices_size;
  vertices_view.StrideInBytes = sizeof(nj_v4_t);

  normals_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress() + total_vertices_size;
  normals_view.SizeInBytes = total_normals_size;
  normals_view.StrideInBytes = sizeof(nj_v4_t);

  // Constant buffer
  res_desc.Width = 1024 * 64;
  DX_CHECKF_RETURN_FALSE(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&constant_buffer)));
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
  cbv_desc.BufferLocation = constant_buffer->GetGPUVirtualAddress();
  // From D3D12HelloConstantBuffers sample
  // CB size is required to be 256-byte aligned
  cbv_desc.SizeInBytes = (sizeof(dx12_cb_t) + 255) & ~255;
  device->CreateConstantBufferView(&cbv_desc, cbv_heap->GetCPUDescriptorHandleForHeapStart());
  DX_CHECKF_RETURN_FALSE(constant_buffer->Map(0, &range, &cbv_p));

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
  nj_cam_update(&cam);
  nj_m4_t perspective = nj_perspective(nj_degree_to_rad(75), width * 1.0f / height, 0.01f, 100.0f);
  cb.mvp = perspective * cam.view_mat;
  cb.cam = nj_v4_t{cam.eye.x, cam.eye.y, cam.eye.z, 0.0f};
  cb.obj_color = nj_v4_t{1.0f, 0.0f, 0.0f, 1.0f};
  cb.light_pos = nj_v4_t{1.3f, 1.3f, 1.3f, 1.0f};
  cb.light_color = nj_v4_t{1.0f, 1.0f, 1.0f, 1.0f};
  memcpy(cbv_p, &cb, sizeof(cb));
  DX_CHECKF_RETURN(cmd_allocators[frame_i]->Reset());
  DX_CHECKF_RETURN(cmd_list->Reset(cmd_allocators[frame_i], pso));

  cmd_list->SetGraphicsRootSignature(root_sig);
  cmd_list->SetDescriptorHeaps(1, &cbv_heap);
  cmd_list->SetGraphicsRootDescriptorTable(0, cbv_heap->GetGPUDescriptorHandleForHeapStart());
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
  D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
  cmd_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);


  float clear_color[] = {0.0f, 0.2f, 0.4f, 1.0f};
  cmd_list->ClearRenderTargetView(rtv_handle, clear_color, 0, NULL);
  cmd_list->ClearDepthStencilView(dsv_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmd_list->IASetVertexBuffers(0, 1, &vertices_view);
  cmd_list->IASetVertexBuffers(1, 1, &normals_view);
  cmd_list->DrawInstanced(total_num_vertices, 1, 0, 0);

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
    WaitForSingleObjectEx(fence_event, INFINITE, FALSE);
  }

  fence_vals[frame_i] = current_fence_val + 1;
}

void nj_dx12_window_t::on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) {
  nj_cam_mouse_event(&cam, mouse, x, y, is_down);
}

void nj_dx12_window_t::on_mouse_move(int x, int y) {
  nj_cam_mouse_move(&cam, x, y);
}

int main(int argc, char** argv) {
  nj_core_init(NJ_OS_LIT("dx12_sample.log"));
  nj_dx12_window_t w(g_general_allocator, NJ_OS_LIT("dx12_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}
