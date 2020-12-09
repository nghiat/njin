//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/allocator.h"
#include "core/compiler.h"
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
#include "core/utils.h"
#include "core/window/window.h"

#include <wchar.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#define DX_CHECKF_RETURN(condition) NJ_CHECKF_RETURN(condition == S_OK, "")
#define DX_CHECKF_RETURN_FALSE(condition) NJ_CHECKF_RETURN_VAL(condition == S_OK, false, "")

#if NJ_IS_CLANG()
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

struct shadow_cb_t {
  nj_m4_t light_mvp;
};

struct rtt_cb_t {
  nj_m4_t mvp;
  nj_m4_t light_mvp;
  nj_v4_t cam;
  nj_v4_t obj_color;
  nj_v4_t light_pos;
  nj_v4_t light_color;
};

struct dx12_descriptor_heap {
  ID3D12DescriptorHeap* heap = NULL;
  nju32 increment_size = 0;
  nju32 curr_index = 0;
};

struct dx12_descriptor {
  dx12_descriptor_heap* descriptor_heap = NULL;
  nju32 index = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
};

struct dx12_buffer {
  ID3D12Resource* buffer = NULL;
  void* cpu_p = NULL;
  njsp offset = 0;
};

struct dx12_subbuffer {
  dx12_buffer* buffer = NULL;
  nju8* cpu_p = NULL;
  D3D12_GPU_VIRTUAL_ADDRESS gpu_p = 0;
  njsp size = 0;
};

struct dx12_window_t : public nj_window_t {
  dx12_window_t(nj_allocator_t* allocator, const nj_os_char* title, int w, int h) : nj_window_t(allocator, title, w, h) {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;

  void wait_for_gpu();

  nj_cam_t m_cam;

  static const int sc_frame_count = 2;
  int m_frame_no;
  ID3D12Device* m_device = NULL;
  ID3D12CommandQueue* m_cmd_queue = NULL;
  IDXGISwapChain3* m_swap_chain = NULL;

  dx12_descriptor_heap m_rtv_heap;
  ID3D12Resource* m_render_targets[sc_frame_count];
  dx12_descriptor m_rtv_descriptors[sc_frame_count];

  ID3D12CommandAllocator* m_cmd_allocators[sc_frame_count];
  ID3D12GraphicsCommandList* m_cmd_list;

  dx12_descriptor_heap m_cbv_srv_heap;
  dx12_buffer m_const_buffer;
  dx12_descriptor m_shadow_srv_descriptor;
  dx12_descriptor m_rtt_cbv_descriptor;
  dx12_subbuffer m_rtt_cb_subbuffer;
  rtt_cb_t m_rtt_cb = {};
  dx12_descriptor m_shadow_cbv_descriptor;
  dx12_subbuffer m_shadow_cb_subbuffer;
  shadow_cb_t m_shadow_cb;

  dx12_descriptor_heap m_dsv_heap;
  ID3D12Resource* m_depth_stencil;
  ID3D12Resource* m_shadow_depth_stencil;
  dx12_descriptor m_depth_rt_descriptor;
  dx12_descriptor m_shadow_depth_rt_descriptor;

  ID3D12RootSignature* m_shadow_root_sig;
  ID3D12RootSignature* m_rtt_root_sig;
  ID3DBlob* m_rtt_vs;
  ID3DBlob* m_rtt_ps;
  ID3DBlob* m_shadow_vs;
  // ID3DBlob* m_shadow_ps;

  ID3D12PipelineState* m_shadow_pso;
  ID3D12PipelineState* m_rtt_pso;

  ID3D12Resource* m_vertex_buffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertices_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_normals_vb_view;
  nju32 m_obj_vertices_num = 0;

  const nju32 m_normals_stride = 64 * 1024 * 1024;

  ID3D12Fence* m_fence;
  HANDLE m_fence_event;
  nju64 m_fence_vals[sc_frame_count] = {};
};

static bool create_descriptor_heap(dx12_descriptor_heap* descriptor_heap, ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, nju32 max_descriptors_num) {
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = type;
  desc.NumDescriptors = max_descriptors_num;
  desc.Flags = flags;
  desc.NodeMask = 0;
  DX_CHECKF_RETURN_FALSE(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap->heap)));
  descriptor_heap->increment_size = device->GetDescriptorHandleIncrementSize(type);
  return true;
}

static dx12_descriptor allocate_descriptor(dx12_descriptor_heap* descriptor_heap) {
  dx12_descriptor descriptor;
  NJ_CHECKF_RETURN_VAL(descriptor_heap->curr_index < descriptor_heap->heap->GetDesc().NumDescriptors, descriptor, "Out of descriptors");
  descriptor.descriptor_heap = descriptor_heap;
  descriptor.index = descriptor_heap->curr_index;
  descriptor.cpu_handle.ptr = descriptor_heap->heap->GetCPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  descriptor.gpu_handle.ptr = descriptor_heap->heap->GetGPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  ++descriptor_heap->curr_index;
  return descriptor;
}

static D3D12_RESOURCE_DESC create_resource_desc(size_t size) {
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment = 0;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  return desc;
}

static D3D12_HEAP_PROPERTIES create_heap_props(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES props = {};
  props.Type = type;
  props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  props.CreationNodeMask = 1;
  props.VisibleNodeMask = 1;
  return props;
}

static D3D12_DESCRIPTOR_RANGE1 create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE range_type,
                                                           UINT num_descriptors,
                                                           UINT base_shader_reg,
                                                           UINT reg_space,
                                                           D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                                           UINT offset_in_descriptors_from_table_start = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
  D3D12_DESCRIPTOR_RANGE1 range = {};
  range.RangeType = range_type;
  range.NumDescriptors = num_descriptors;
  range.BaseShaderRegister = base_shader_reg;
  range.RegisterSpace = reg_space;
  range.Flags = flags;
  range.OffsetInDescriptorsFromTableStart = offset_in_descriptors_from_table_start;
  return range;
}

static D3D12_ROOT_PARAMETER1
create_root_param_1_1_descriptor_table(UINT num_ranges, const D3D12_DESCRIPTOR_RANGE1* ranges, D3D12_SHADER_VISIBILITY shader_visibility) {
  D3D12_ROOT_PARAMETER1 param = {};
  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  param.DescriptorTable = {};
  param.DescriptorTable.NumDescriptorRanges = num_ranges;
  param.DescriptorTable.pDescriptorRanges = ranges;
  param.ShaderVisibility = shader_visibility;
  return param;
}

static D3D12_VERSIONED_ROOT_SIGNATURE_DESC create_root_sig_desc_1_1(UINT num_root_params,
                                                                    const D3D12_ROOT_PARAMETER1* root_params,
                                                                    UINT num_static_samplers,
                                                                    const D3D12_STATIC_SAMPLER_DESC* static_samplers,
                                                                    D3D12_ROOT_SIGNATURE_FLAGS flags) {
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
  desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  desc.Desc_1_1.NumParameters = num_root_params;
  desc.Desc_1_1.pParameters = root_params;
  desc.Desc_1_1.NumStaticSamplers = num_static_samplers;
  desc.Desc_1_1.pStaticSamplers = static_samplers;
  desc.Desc_1_1.Flags = flags;
  return desc;
}

static D3D12_RESOURCE_BARRIER create_transition_barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  return barrier;
}

static D3D12_CONSTANT_BUFFER_VIEW_DESC create_const_buf_view_desc(D3D12_GPU_VIRTUAL_ADDRESS location, UINT size) {
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  desc.BufferLocation = location;
  desc.SizeInBytes = size;
  return desc;
}

static bool create_buffer(dx12_buffer* buffer, ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap_props, const D3D12_RESOURCE_DESC* desc) {
  *buffer = {};
  DX_CHECKF_RETURN_FALSE(device->CreateCommittedResource(heap_props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buffer->buffer)));
  buffer->buffer->Map(0, NULL, &buffer->cpu_p);
  return true;
}

static dx12_subbuffer allocate_const_buffer(dx12_buffer* buffer, njsp size) {
  dx12_subbuffer subbuffer = {};
  NJ_CHECKF_RETURN_VAL(buffer->offset + size <= buffer->buffer->GetDesc().Width, subbuffer, "Out of memory");
  subbuffer.buffer = buffer;
  subbuffer.cpu_p = (nju8*)buffer->cpu_p + buffer->offset;
  subbuffer.gpu_p = buffer->buffer->GetGPUVirtualAddress() + buffer->offset;
  subbuffer.size = size;
  buffer->offset += size;
  return subbuffer;
}

bool dx12_window_t::init() {
  nj_window_t::init();

  nj_cam_init(&m_cam, {5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
  m_rtt_cb.cam = nj_v3_to_v4(m_cam.eye, 1.0f);
  m_rtt_cb.obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
  m_rtt_cb.light_pos = {10.0f, 10.0f, 10.0f, 1.0f};
  m_rtt_cb.light_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // The light is static for now.
  nj_cam_t light_cam;
  nj_cam_init(&light_cam, {5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
  // TODO: ortho?
  nj_m4_t perspective = nj_perspective(nj_degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);
  m_shadow_cb.light_mvp = perspective * light_cam.view_mat;
  m_rtt_cb.light_mvp = perspective * light_cam.view_mat;

  UINT dxgi_factory_flags = 0;
  {
    // Enable debug layer.
    ID3D12Debug* debug_controller;
    DX_CHECKF_RETURN_FALSE(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer();
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    debug_controller->Release();
  }

  IDXGIFactory4* dxgi_factory;
  DX_CHECKF_RETURN_FALSE(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

  {
    // Choose adapter (graphics card).
    IDXGIAdapter1* adapter;
    int backup_adapter_i = -1;
    int adapter_i = -1;
    for (int i = 0; dxgi_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        continue;

      if (wcsstr(desc.Description, L"Intel")) {
        backup_adapter_i = i;
        continue;
      }

      adapter_i = i;
      break;
    }

    if (adapter_i == -1) {
      NJ_CHECKF_RETURN_VAL(backup_adapter_i != -1, false, "Can't find a dx12 adapter");
      adapter_i = backup_adapter_i;
    }

    dxgi_factory->EnumAdapters1(adapter_i, &adapter);
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    NJ_LOGI("%ls", desc.Description);
    DX_CHECKF_RETURN_FALSE(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
  }

  {
    D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
    cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    DX_CHECKF_RETURN_FALSE(m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue)));
  }

  DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
  sc_desc.BufferCount = sc_frame_count;
  sc_desc.Width = m_width;
  sc_desc.Height = m_height;
  sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sc_desc.SampleDesc.Count = 1;

  IDXGISwapChain1* swap_chain;
  DX_CHECKF_RETURN_FALSE(dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue, *(HWND*)m_handle, &sc_desc, NULL, NULL, &swap_chain));
  DX_CHECKF_RETURN_FALSE(dxgi_factory->MakeWindowAssociation(*(HWND*)m_handle, DXGI_MWA_NO_ALT_ENTER));
  DX_CHECKF_RETURN_FALSE(swap_chain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)));
  dxgi_factory->Release();
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();

  {
    if (!create_descriptor_heap(&m_rtv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, sc_frame_count))
      return false;

    for (int i = 0; i < sc_frame_count; ++i) {
      DX_CHECKF_RETURN_FALSE(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
      m_rtv_descriptors[i] = allocate_descriptor(&m_rtv_heap);
      m_device->CreateRenderTargetView(m_render_targets[i], NULL, m_rtv_descriptors[i].cpu_handle);
    }
  }

  {
    if (!create_descriptor_heap(&m_dsv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2))
      return false;

    D3D12_RESOURCE_DESC depth_tex_desc = {};
    depth_tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_tex_desc.Alignment = 0;
    depth_tex_desc.Width = m_width;
    depth_tex_desc.Height = m_height;
    depth_tex_desc.DepthOrArraySize = 1;
    depth_tex_desc.MipLevels = 1;
    depth_tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depth_tex_desc.SampleDesc.Count = 1;
    depth_tex_desc.SampleDesc.Quality = 0;
    depth_tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_props = create_heap_props(D3D12_HEAP_TYPE_DEFAULT);

    DX_CHECKF_RETURN_FALSE(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&m_depth_stencil)));
    m_depth_rt_descriptor = allocate_descriptor(&m_dsv_heap);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_view_desc = {};
    dsv_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_view_desc.Texture2D.MipSlice = 0;
    m_device->CreateDepthStencilView(m_depth_stencil, &dsv_view_desc, m_depth_rt_descriptor.cpu_handle);

    DX_CHECKF_RETURN_FALSE(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_GENERIC_READ, &clear_value, IID_PPV_ARGS(&m_shadow_depth_stencil)));
    m_shadow_depth_rt_descriptor = allocate_descriptor(&m_dsv_heap);
    m_device->CreateDepthStencilView(m_shadow_depth_stencil, &dsv_view_desc, m_shadow_depth_rt_descriptor.cpu_handle);
  }

  {
    // 1 srv for shadow rt
    // 2 cbv
    if(!create_descriptor_heap(&m_cbv_srv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 3))
      return false;

    m_shadow_srv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
    D3D12_SHADER_RESOURCE_VIEW_DESC shadow_srv_desc = {};
    shadow_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shadow_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shadow_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shadow_srv_desc.Texture2D.MostDetailedMip = 0;
    shadow_srv_desc.Texture2D.MipLevels = 1;
    shadow_srv_desc.Texture2D.PlaneSlice = 0;
    shadow_srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
    m_device->CreateShaderResourceView(m_shadow_depth_stencil, &shadow_srv_desc, m_shadow_srv_descriptor.cpu_handle);

    // Constant buffer
    D3D12_RESOURCE_DESC cbv_res_desc = create_resource_desc(64 * 1024);
    D3D12_HEAP_PROPERTIES heap_props = create_heap_props(D3D12_HEAP_TYPE_UPLOAD);
    if (!create_buffer(&m_const_buffer, m_device, &heap_props, &cbv_res_desc))
      return false;
    // From D3D12HelloConstantBuffers sample
    // CB size is required to be 256-byte aligned
    m_rtt_cbv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
    njsp cb_size = (sizeof(rtt_cb_t) + 255) & ~255;
    m_rtt_cb_subbuffer = allocate_const_buffer(&m_const_buffer, cb_size);
    m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_rtt_cb_subbuffer.gpu_p, m_rtt_cb_subbuffer.size), m_rtt_cbv_descriptor.cpu_handle);

    m_shadow_cbv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
    cb_size = (sizeof(shadow_cb_t) + 255) & ~255;
    m_shadow_cb_subbuffer = allocate_const_buffer(&m_const_buffer, cb_size);
    m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_shadow_cb_subbuffer.gpu_p, m_shadow_cb_subbuffer.size), m_shadow_cbv_descriptor.cpu_handle);
  }

  {
    // Create a root signature consisting of a descriptor table with a single CBV
    D3D12_DESCRIPTOR_RANGE1 ranges = create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
    D3D12_ROOT_PARAMETER1 root_params = create_root_param_1_1_descriptor_table(1, &ranges, D3D12_SHADER_VISIBILITY_ALL);
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = create_root_sig_desc_1_1(
        1,
        &root_params,
        0,
        NULL,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
    ID3DBlob* signature;
    ID3DBlob* error;
    DX_CHECKF_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
    DX_CHECKF_RETURN_FALSE(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_shadow_root_sig)));

    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    nj_os_char shader_path[NJ_MAX_PATH];
    nj_path_from_exe_dir(NJ_OS_LIT("assets/shadow.hlsl"), shader_path, NJ_MAX_PATH);
    DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &m_shadow_vs, NULL));
    // DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &m_shadow_ps, NULL));

    nj_path_from_exe_dir(NJ_OS_LIT("assets/shader.hlsl"), shader_path, NJ_MAX_PATH);
    DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &m_rtt_vs, NULL));
    DX_CHECKF_RETURN_FALSE(D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &m_rtt_ps, NULL));
  }

  {
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
    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
      D3D12_RENDER_TARGET_BLEND_DESC* rt_desc = &blend_desc.RenderTarget[i];
      *rt_desc = {};
      rt_desc->BlendEnable = FALSE;
      rt_desc->LogicOpEnable = FALSE;
      rt_desc->SrcBlend = D3D12_BLEND_ONE;
      rt_desc->DestBlend = D3D12_BLEND_ONE;
      rt_desc->BlendOp = D3D12_BLEND_OP_ADD;
      rt_desc->SrcBlendAlpha = D3D12_BLEND_ONE;
      rt_desc->DestBlendAlpha =D3D12_BLEND_ONE ;
      rt_desc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
      rt_desc->LogicOp = D3D12_LOGIC_OP_NOOP;
      rt_desc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    D3D12_INPUT_ELEMENT_DESC input_elem_descs[] = {
      { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC shadow_pso_desc = {};
      shadow_pso_desc.pRootSignature = m_shadow_root_sig;
      shadow_pso_desc.VS.pShaderBytecode = m_shadow_vs->GetBufferPointer();
      shadow_pso_desc.VS.BytecodeLength = m_shadow_vs->GetBufferSize();
      shadow_pso_desc.BlendState = blend_desc;
      shadow_pso_desc.SampleMask = UINT_MAX;
      shadow_pso_desc.RasterizerState = rasterizer_desc;
      shadow_pso_desc.DepthStencilState.DepthEnable = TRUE;
      shadow_pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      shadow_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      shadow_pso_desc.DepthStencilState.StencilEnable = FALSE;
      shadow_pso_desc.InputLayout.pInputElementDescs = input_elem_descs;
      shadow_pso_desc.InputLayout.NumElements = (UINT)nj_static_array_size(input_elem_descs);
      shadow_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      shadow_pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      shadow_pso_desc.NumRenderTargets = 0;
      shadow_pso_desc.SampleDesc.Count = 1;
      DX_CHECKF_RETURN_FALSE(m_device->CreateGraphicsPipelineState(&shadow_pso_desc, IID_PPV_ARGS(&m_shadow_pso)));
    }

    for (int i = 0; i < sc_frame_count; ++i)
      DX_CHECKF_RETURN_FALSE(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocators[i])));
    DX_CHECKF_RETURN_FALSE(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocators[m_frame_no], m_shadow_pso, IID_PPV_ARGS(&m_cmd_list)));
    m_cmd_list->Close();

    D3D12_INPUT_ELEMENT_DESC rtt_elem_descs[] = {
      { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      { "N", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL),
          create_root_param_1_1_descriptor_table(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL),
      };
      D3D12_STATIC_SAMPLER_DESC sampler = {};
      sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.MipLODBias = 0;
      sampler.MaxAnisotropy = 0;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = 0;
      sampler.RegisterSpace = 0;
      sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
      D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc =
          create_root_sig_desc_1_1(nj_static_array_size(root_params), root_params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
      ID3DBlob* signature;
      ID3DBlob* error;
      DX_CHECKF_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
      DX_CHECKF_RETURN_FALSE(
          m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rtt_root_sig)));
    }

    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC rtt_pso_desc = {};
      rtt_pso_desc.pRootSignature = m_rtt_root_sig;
      rtt_pso_desc.VS.pShaderBytecode = m_rtt_vs->GetBufferPointer();
      rtt_pso_desc.VS.BytecodeLength = m_rtt_vs->GetBufferSize();
      rtt_pso_desc.PS.pShaderBytecode = m_rtt_ps->GetBufferPointer();
      rtt_pso_desc.PS.BytecodeLength = m_rtt_ps->GetBufferSize();
      rtt_pso_desc.BlendState = blend_desc;
      rtt_pso_desc.SampleMask = UINT_MAX;
      rtt_pso_desc.RasterizerState = rasterizer_desc;
      rtt_pso_desc.DepthStencilState.DepthEnable = TRUE;
      rtt_pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      rtt_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      rtt_pso_desc.DepthStencilState.StencilEnable = FALSE;
      rtt_pso_desc.InputLayout.pInputElementDescs = rtt_elem_descs;
      rtt_pso_desc.InputLayout.NumElements = (UINT)nj_static_array_size(rtt_elem_descs);
      rtt_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      rtt_pso_desc.NumRenderTargets = 1;
      rtt_pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
      rtt_pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      rtt_pso_desc.SampleDesc.Count = 1;
      DX_CHECKF_RETURN_FALSE(m_device->CreateGraphicsPipelineState(&rtt_pso_desc, IID_PPV_ARGS(&m_rtt_pso)));
    }
  }

  {
    D3D12_RESOURCE_DESC res_desc = {};
    res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    res_desc.Alignment = 0;
    res_desc.Width = 128 * 1024 * 1024;
    res_desc.Height = 1;
    res_desc.DepthOrArraySize = 1;
    res_desc.MipLevels = 1;
    res_desc.Format = DXGI_FORMAT_UNKNOWN;
    res_desc.SampleDesc.Count = 1;
    res_desc.SampleDesc.Quality = 0;
    res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    DX_CHECKF_RETURN_FALSE(m_device->CreateCommittedResource(&create_heap_props(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_vertex_buffer)));
  }

  {
    nj_linear_allocator_t<> temp_allocator("obj_allocator");
    temp_allocator.init();
    const nj_os_char* obj_paths[] = {
        NJ_OS_LIT("assets/plane.obj"),
        NJ_OS_LIT("assets/cube.obj"),
    };
    nju8* vertex_buffer_begin;
    njsp vertices_offset = 0;
    njsp normals_offset = 0;
    DX_CHECKF_RETURN_FALSE(m_vertex_buffer->Map(0, &D3D12_RANGE{0, 0}, (void**)&vertex_buffer_begin));
    int obj_count = nj_static_array_size(obj_paths);
    for (int i = 0; i < obj_count; ++i) {
      nj_obj_t obj;
      nj_os_char full_obj_path[NJ_MAX_PATH];
      nj_obj_init(&obj, &temp_allocator, nj_path_from_exe_dir(obj_paths[i], full_obj_path, NJ_MAX_PATH));
      int obj_len = nj_da_len(&obj.vertices);
      int vertices_size = obj_len * sizeof(obj.vertices[0]);
      int normals_size = obj_len * sizeof(obj.normals[0]);
      m_obj_vertices_num += obj_len;
      memcpy(vertex_buffer_begin + vertices_offset, &obj.vertices[0], vertices_size);
      memcpy(vertex_buffer_begin + m_normals_stride + normals_offset, &obj.normals[0], normals_size);
      vertices_offset += vertices_size;
      normals_offset += normals_size;
    }
    temp_allocator.destroy();
    m_vertices_vb_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
    m_vertices_vb_view.SizeInBytes = vertices_offset;
    m_vertices_vb_view.StrideInBytes = sizeof(((nj_obj_t*)0)->vertices[0]);

    m_normals_vb_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress() + m_normals_stride;
    m_normals_vb_view.SizeInBytes = normals_offset;
    m_normals_vb_view.StrideInBytes = sizeof(((nj_obj_t*)0)->normals[0]);

    m_vertex_buffer->Unmap(0, NULL);
  }

  {
    DX_CHECKF_RETURN_FALSE(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_vals[m_frame_no];
    m_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    NJ_CHECKF_RETURN_VAL(m_fence_event, false, "");
    wait_for_gpu();
  }

  return true;
}

void dx12_window_t::destroy() {
  if (m_device)
    m_device->Release();
  if (m_cmd_queue)
    m_cmd_queue->Release();
}

void dx12_window_t::loop() {
  memcpy(m_shadow_cb_subbuffer.cpu_p, &m_shadow_cb, sizeof(m_shadow_cb));

  nj_cam_update(&m_cam);
  nj_m4_t perspective = nj_perspective(nj_degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);
  m_rtt_cb.mvp = perspective * m_cam.view_mat;
  memcpy(m_rtt_cb_subbuffer.cpu_p, &m_rtt_cb, sizeof(m_rtt_cb));

  DX_CHECKF_RETURN(m_cmd_allocators[m_frame_no]->Reset());
  DX_CHECKF_RETURN(m_cmd_list->Reset(m_cmd_allocators[m_frame_no], m_shadow_pso));

  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = (float)m_width;
  viewport.Height = (float)m_height;
  viewport.MinDepth = D3D12_MIN_DEPTH;
  viewport.MaxDepth = D3D12_MAX_DEPTH;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0;
  scissor_rect.top = 0;
  scissor_rect.right = m_width;
  scissor_rect.bottom = m_height;
  m_cmd_list->RSSetViewports(1, &viewport);
  m_cmd_list->RSSetScissorRects(1, &scissor_rect);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

  m_cmd_list->SetPipelineState(m_shadow_pso);
  m_cmd_list->SetGraphicsRootSignature(m_shadow_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_shadow_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(0, NULL, FALSE, &m_shadow_depth_rt_descriptor.cpu_handle);
  m_cmd_list->ClearDepthStencilView(m_shadow_depth_rt_descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_vertices_vb_view);
  m_cmd_list->DrawInstanced(m_obj_vertices_num, 1, 0, 0);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

  m_cmd_list->SetPipelineState(m_rtt_pso);
  m_cmd_list->SetGraphicsRootSignature(m_rtt_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_shadow_srv_descriptor.gpu_handle);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_rtt_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(1, &m_rtv_descriptors[m_frame_no].cpu_handle, FALSE, &m_depth_rt_descriptor.cpu_handle);
  float clear_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
  m_cmd_list->ClearRenderTargetView(m_rtv_descriptors[m_frame_no].cpu_handle, clear_color, 0, NULL);
  m_cmd_list->ClearDepthStencilView(m_depth_rt_descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_vertices_vb_view);
  m_cmd_list->IASetVertexBuffers(1, 1, &m_normals_vb_view);
  m_cmd_list->DrawInstanced(m_obj_vertices_num, 1, 0, 0);
  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  m_cmd_list->Close();
  m_cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_cmd_list);
  DX_CHECKF_RETURN(m_swap_chain->Present(1, 0));
  // Prepare to render the next frame
  nju64 curr_fence_val = m_fence_vals[m_frame_no];
  DX_CHECKF_RETURN(m_cmd_queue->Signal(m_fence, curr_fence_val));
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();
  // If the next frame is not ready to be rendered yet, wait until it's ready.
  if (m_fence->GetCompletedValue() < m_fence_vals[m_frame_no]) {
    DX_CHECKF_RETURN(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
    WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
  }
  m_fence_vals[m_frame_no] = curr_fence_val + 1;
}

void dx12_window_t::on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) {
  nj_cam_mouse_event(&m_cam, mouse, x, y, is_down);
}

void dx12_window_t::on_mouse_move(int x, int y) {
  nj_cam_mouse_move(&m_cam, x, y);
}

void dx12_window_t::wait_for_gpu() {
  // Wait for pending GPU work to complete.

  // Schedule a Signal command in the queue.
  DX_CHECKF_RETURN(m_cmd_queue->Signal(m_fence, m_fence_vals[m_frame_no]));

  // Wait until the fence has been processed.
  DX_CHECKF_RETURN(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
  WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);

  // Increment the fence value for the current frame.
  ++m_fence_vals[m_frame_no];
}

int main() {
  nj_core_init(NJ_OS_LIT("dx12_sample.log"));
  dx12_window_t w(g_general_allocator, NJ_OS_LIT("dx12_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}

#if NJ_IS_CLANG()
#pragma clang diagnostic pop
#endif
