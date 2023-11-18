#include "DX12Graphics.h"

#include "DX12DescriptorAllocator.h"
#include "DX12Utils.h"
#include "DX12Resources.h"
#include "DX12ShaderResource.h"

#include "FileSystem/FileUtils.h"

#include "Runtime/StartupConfig.h"

#include "Core/Thread/Thread.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"


NAMESPACE_DX12_BEGIN

DX12Graphics::DX12Graphics(void* hwnd)
{
    InitD3D12();
    InitAllocators();
    InitSwapchain(hwnd);
    InitFence();
    m_ringBufferCmdList.Resize(m_device.Get(), NUM_GRAPHICS_COMMAND_LIST_ALLOCATORS);
    InitRootSignature(); 
    InitGPUVisibleDescriptorHeap();
    InitImGui(hwnd);

    m_compiledShadersPath = StartupConfig::Get().compiledShadersPath;
}

DX12Graphics::~DX12Graphics()
{
    ExecuteCurrentCmdList();

    m_currentRenderTarget.m_rtv.ptr = 0;
    m_currentRenderTarget.m_fenceValue = 0;
    ((DX12ShaderResource*)m_currentRenderTarget.m_shaderResource.get())->m_srvGroupStart.ptr = 0;
    ((DX12ShaderResource*)m_currentRenderTarget.m_shaderResource.get())->m_lastFenceValue = 0;

    m_currentDepthStencilBuffer.m_fenceValue = 0;
    m_currentDepthStencilBuffer.m_dsv.ptr = 0;
    ((DX12ShaderResource*)m_currentDepthStencilBuffer.m_shaderResource.get())->m_srvGroupStart.ptr = 0;
    ((DX12ShaderResource*)m_currentDepthStencilBuffer.m_shaderResource.get())->m_lastFenceValue = 0;

    while (!m_waitForFreeResources.empty())
    {
        ProcessFreeDX12ResourceList();
        Thread::Sleep(60);
    }

    SignalCurrentDX12FenceValue();
    if (m_currentFenceValue != 0)
    {
        WaitForDX12FenceValue(m_currentFenceValue - 1);
    }

    // ImGui Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void DX12Graphics::FirstInit()
{
    m_resourceUploader.Initialize();

    auto cmdList = GetCmdList();
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_gpuVisibleHeap.Get() };
    cmdList->SetDescriptorHeaps(1, ppHeaps);
}

void DX12Graphics::InitD3D12()
{
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif // _DEBUG

    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    m_dxgiFactory = dxgiFactory;

    uint32_t i = 0;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIAdapter> adapters[32] = {};
    while (dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        adapters[i++] = adapter;
    }


    uint32_t maxDedicatedMemory = 0;
    uint32_t chosenAdapterId = 0;
    i = 0;
    DXGI_ADAPTER_DESC adapterDesc = {};
    for (auto& adp : adapters)
    {
        if (adp.Get() == nullptr) break;

        ThrowIfFailed(adp->GetDesc(&adapterDesc));

        if (SUCCEEDED(D3D12CreateDevice(adp.Get(), D3D_FEATURE_LEVEL_11_1, __uuidof(ID3D12Device), 0))
            && maxDedicatedMemory <= adapterDesc.DedicatedVideoMemory)
        {
            chosenAdapterId = i;
            maxDedicatedMemory = adapterDesc.DedicatedVideoMemory;
        }

        ++i;
    }

    ThrowIfFailed(D3D12CreateDevice(adapters[chosenAdapterId].Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device)));
    m_adapter = adapters[chosenAdapterId];

    m_CBV_SRV_UAV_CPUdescriptorHandleStride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DX12Graphics::InitAllocators()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = RTV_ALLOCATOR_NUM_RTV_PER_HEAP;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    m_rtvAllocator.Initialize(m_device.Get(), rtvDescriptorHeapDesc);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = DSV_ALLOCATOR_NUM_DSV_PER_HEAP;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    m_dsvAllocator.Initialize(m_device.Get(), dsvDescriptorHeapDesc);

    D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
    srvDescriptorHeapDesc.NumDescriptors = SRV_ALLOCATOR_NUM_SRV_PER_HEAP;
    srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    m_srvAllocator.Initialize(m_device.Get(), srvDescriptorHeapDesc);

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = m_device.Get();
    allocatorDesc.pAdapter = m_adapter.Get();
    ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &m_dx12ResourceAllocator));
}

void DX12Graphics::InitSwapchain(void* _hwnd)
{
    RECT displayRect;
    auto hwnd = (HWND)_hwnd;
    if (!GetClientRect(hwnd, &displayRect))
    {
        assert(0);
        exit(-1);
    }

    const auto WINDOW_WIDTH = displayRect.right - displayRect.left;
    const auto WINDOW_HEIGHT = displayRect.bottom - displayRect.top;

    // create command queue
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    ThrowIfFailed(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));


    // create swapchain
    ComPtr<IDXGISwapChain> tempSwapChain;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = NUM_GRAPHICS_BACK_BUFFERS;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferDesc.Format = BACK_BUFFER_FORMAT;
    swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;
    swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &tempSwapChain));
    ThrowIfFailed(tempSwapChain.As(&m_swapChain));
    m_currentBackBufferId = m_swapChain->GetCurrentBackBufferIndex();

    //ThrowIfFailed(m_swapChain->SetMaximumFrameLatency(1));

    auto rtShaderResource = std::make_shared<DX12ShaderResource>();
    rtShaderResource->m_resource.resource = m_renderTargets[0];
    rtShaderResource->m_srvGroupStart = GetSRVAllocator()->AllocateCPU(NUM_GRAPHICS_BACK_BUFFERS);
    rtShaderResource->m_srvGroupCount = NUM_GRAPHICS_BACK_BUFFERS;
    rtShaderResource->m_srv = rtShaderResource->m_srvGroupStart;
    m_currentRenderTarget.m_shaderResource = rtShaderResource;
    m_currentRenderTarget.m_currentState = D3D12_RESOURCE_STATE_COMMON;

    auto rtSRVStart = rtShaderResource->m_srvGroupStart;
    auto rtSRVStride = GetSRVAllocator()->GetCPUStride();

    D3D12_SHADER_RESOURCE_VIEW_DESC rtSRVDesc = {};
    rtSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    rtSRVDesc.Format = swapChainDesc.BufferDesc.Format;
    rtSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    rtSRVDesc.Texture2D.MipLevels = 1;

    // create rt for each back buffer
    auto cpuHandle = m_rtvAllocator.AllocateCPU(NUM_GRAPHICS_BACK_BUFFERS);
    auto stride = m_rtvAllocator.GetCPUStride();
    m_rtvStart = cpuHandle;
    for (size_t i = 0; i < NUM_GRAPHICS_BACK_BUFFERS; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), 0, cpuHandle);
        cpuHandle.ptr += stride;

        m_device->CreateShaderResourceView(m_renderTargets[i].Get(), &rtSRVDesc, rtSRVStart);
        rtSRVStart.ptr += rtSRVStride;
    }


    // create depth buffer for each back buffer
    D3D12_RESOURCE_DESC depthBufferDesc = {};
    depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthBufferDesc.Width = WINDOW_WIDTH;
    depthBufferDesc.Height = WINDOW_HEIGHT;
    depthBufferDesc.DepthOrArraySize = 1;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    cpuHandle = m_dsvAllocator.AllocateCPU(NUM_GRAPHICS_BACK_BUFFERS);
    stride = m_dsvAllocator.GetCPUStride();
    m_dsvStart = cpuHandle;

    auto dsShaderResource = std::make_shared<DX12ShaderResource>();
    dsShaderResource->m_resource.resource = m_renderTargets[0];
    dsShaderResource->m_srvGroupStart = GetSRVAllocator()->AllocateCPU(NUM_GRAPHICS_BACK_BUFFERS);
    dsShaderResource->m_srvGroupCount = NUM_GRAPHICS_BACK_BUFFERS;
    dsShaderResource->m_srv = dsShaderResource->m_srvGroupStart;
    m_currentDepthStencilBuffer.m_shaderResource = dsShaderResource;

    auto dsSRVStart = dsShaderResource->m_srvGroupStart;
    auto dsSRVStride = GetSRVAllocator()->GetCPUStride();

    D3D12_SHADER_RESOURCE_VIEW_DESC dsSRVDesc = {};
    dsSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    dsSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    dsSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    dsSRVDesc.Texture2D.MipLevels = 1;

    for (size_t i = 0; i < NUM_GRAPHICS_BACK_BUFFERS; i++)
    {
        AllocateDX12Resource(
            &depthBufferDesc,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            &m_depthBuffers[i]
        );

        m_device->CreateDepthStencilView(m_depthBuffers[i].resource.Get(), &dsvDesc, cpuHandle);
        cpuHandle.ptr += stride;

        m_device->CreateShaderResourceView(m_depthBuffers[i].resource.Get(), &dsSRVDesc, dsSRVStart);
        dsSRVStart.ptr += dsSRVStride;
    }

    m_currentDepthStencilBuffer.m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    m_backBufferViewport.TopLeftX = 0;
    m_backBufferViewport.TopLeftY = 0;
    m_backBufferViewport.MinDepth = 0.0f;
    m_backBufferViewport.MaxDepth = 1.0f;
    m_backBufferViewport.Width = WINDOW_WIDTH;
    m_backBufferViewport.Height = WINDOW_HEIGHT;

    m_backBufferScissorRect.top = 0;
    m_backBufferScissorRect.left = 0;
    m_backBufferScissorRect.right = WINDOW_WIDTH;
    m_backBufferScissorRect.bottom = WINDOW_HEIGHT;

    //m_swapChain->SetFullscreenState(true, nullptr);

    m_windowWidth = WINDOW_WIDTH;
    m_windowHeight = WINDOW_HEIGHT;
}

void DX12Graphics::InitFence()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_currentFenceValue = 1;
}

void DX12Graphics::ExecuteCurrentCmdList()
{
    auto cmdList = GetCmdList();
    cmdList->Close();

    ID3D12CommandList* ppCommandLists[] = { cmdList };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(
        m_commandQueue->Signal(
            m_fence.Get(),
            m_currentFenceValue++
        )
    );

    m_ringBufferCmdList.NextCmdListAlloc(m_currentFenceValue - 1, m_fence.Get(), m_fenceEvent);

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_gpuVisibleHeap.Get() };
    cmdList->SetDescriptorHeaps(1, ppHeaps);

    if (m_currentGraphicsPipeline)
    {
        m_currentGraphicsPipeline->m_lastFenceValue = GetCurrentDX12FenceValue();
        cmdList->SetPipelineState(m_currentDX12GraphicsPipeline.Get());
    }

    if (m_numBoundRTVs)
    {
        cmdList->OMSetRenderTargets(m_numBoundRTVs, m_currentBoundRTVs, 0, m_currentBoundDSV.ptr == 0 ? 0 : &m_currentBoundDSV);
        cmdList->RSSetViewports(1, &m_backBufferViewport); // set the viewports
        cmdList->RSSetScissorRects(1, &m_backBufferScissorRect); // set the scissor rects
    }
}

void DX12Graphics::ThreadSafeFreeDX12Resource(const DX12Resource& resource, UINT64 fenceValue)
{
    WaitForFreeDX12Resource rc;
    rc.allocation = resource.allocation;
    rc.dx12Object = resource.resource;
    rc.fenceValue = fenceValue;

    m_waitForFreeResourcesLock.lock();
    m_waitForFreeResources.push_back(rc);
    m_waitForFreeResourcesLock.unlock();
}

void DX12Graphics::ThreadSafeFreeDX12Resource(ComPtr<ID3D12Object> resource, UINT64 fenceValue)
{
    WaitForFreeDX12Resource rc;
    rc.allocation = nullptr;
    rc.dx12Object = resource;
    rc.fenceValue = fenceValue;

    m_waitForFreeResourcesLock.lock();
    m_waitForFreeResources.push_back(rc);
    m_waitForFreeResourcesLock.unlock();
}

void DX12Graphics::ProcessFreeDX12ResourceList()
{
    if (m_waitForFreeResources.empty()) return;
    
    m_waitForFreeResourcesLock.lock();

    std::sort(m_waitForFreeResources.begin(), m_waitForFreeResources.end(),
        [](const WaitForFreeDX12Resource& a, const WaitForFreeDX12Resource& b)
        {
            return a.fenceValue > b.fenceValue;
        }
    );

    auto completedValue = m_fence->GetCompletedValue();
    int i = m_waitForFreeResources.size() - 1;
    for (; i != -1; i--)
    {
        auto& rc = m_waitForFreeResources[i];

        if (completedValue < rc.fenceValue)
        {
            // no wait
            break;
        }

        rc = {};
        m_waitForFreeResources.pop_back();
    }
    
    m_waitForFreeResourcesLock.unlock();
}

SharedPtr<GraphicsPipeline> DX12Graphics::CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc)
{
    size_t numRenderCallPerFrame = std::clamp(desc.preferRenderCallPerFrame, GRAPHICS_PIPELINE_MIN_RENDER_ROOMS, GRAPHICS_PIPELINE_MAX_RENDER_ROOMS);
    auto ret = std::make_shared<DX12GraphicsPipeline>(numRenderCallPerFrame, desc);
    auto& dx12pipeline = ret->m_pipelineState;

    byte *vs = nullptr, *ps = nullptr, *gs = nullptr, *hs = nullptr, *ds = nullptr;
    size_t lenVS = 0, lenPS = 0, lenGS = 0, lenHS = 0, lenDS = 0;

    if (desc.vs)
    {
        FileUtils::ReadFile(m_compiledShadersPath + String(desc.vs) + ".cso", vs, lenVS);
    }

    if (desc.ps)
    {
        FileUtils::ReadFile(m_compiledShadersPath + String(desc.ps) + ".cso", ps, lenPS);
    }

    if (desc.gs)
    {
        FileUtils::ReadFile(m_compiledShadersPath + String(desc.gs) + ".cso", gs, lenGS);
    }

    if (desc.hs)
    {
        FileUtils::ReadFile(m_compiledShadersPath + String(desc.hs) + ".cso", hs, lenHS);
    }

    if (desc.ds)
    {
        FileUtils::ReadFile(m_compiledShadersPath + String(desc.ds) + ".cso", ds, lenDS);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC dx12desc = {};

    dx12desc.pRootSignature = m_rootSignature.Get();

    D3D12_INPUT_ELEMENT_DESC inputElements[16] = {};

    for (size_t i = 0; i < desc.inputDesc.numElements; i++)
    {
        inputElements[i] = dx12utils::ConvertToDX12InputElement(desc.inputDesc.elements[i]);
    }

    dx12desc.InputLayout.NumElements = desc.inputDesc.numElements;
    dx12desc.InputLayout.pInputElementDescs = inputElements;

    dx12desc.VS.pShaderBytecode = vs;
    dx12desc.VS.BytecodeLength = lenVS;

    dx12desc.PS.pShaderBytecode = ps;
    dx12desc.PS.BytecodeLength = lenPS;

    dx12desc.GS.pShaderBytecode = gs;
    dx12desc.GS.BytecodeLength = lenGS;

    dx12desc.HS.pShaderBytecode = hs;
    dx12desc.HS.BytecodeLength = lenHS;

    dx12desc.DS.pShaderBytecode = ds;
    dx12desc.DS.BytecodeLength = lenDS;

    dx12desc.PrimitiveTopologyType = dx12utils::ConvertToDX12PrimitiveTopoplogy(desc.primitiveTopology); // type of topology we are drawing
    ret->m_primitiveTopology = dx12utils::ConvertToD3DPrimitiveTopoplogy(dx12desc.PrimitiveTopologyType);
    dx12desc.SampleDesc.Count = 1;
    dx12desc.SampleDesc.Quality = 0; // must be the same sample description as the swapchain and depth/stencil buffer
    dx12desc.SampleMask = 0xffffffff;
    dx12desc.NumRenderTargets = desc.outputDesc.numRenderTarget;
    for (size_t i = 0; i < desc.outputDesc.numRenderTarget; i++)
    {
        dx12desc.RTVFormats[i] = dx12utils::ConvertToDX12Format(desc.outputDesc.RTVFormat[i]); // format of the render target
    }

    dx12desc.RasterizerState.FillMode = dx12utils::ConvertToDX12FillMode(desc.rasterizerDesc.fillMode);
    dx12desc.RasterizerState.CullMode = dx12utils::ConvertToDX12CullMode(desc.rasterizerDesc.cullMode);
    dx12desc.RasterizerState.FrontCounterClockwise = desc.rasterizerDesc.frontCounterClockwise;
    dx12desc.RasterizerState.DepthBias = desc.rasterizerDesc.depthBias;
    dx12desc.RasterizerState.SlopeScaledDepthBias = desc.rasterizerDesc.slopeScaledDepthBias;
    dx12desc.RasterizerState.DepthBiasClamp = desc.rasterizerDesc.depthBiasClamp;
    dx12desc.RasterizerState.DepthClipEnable = desc.rasterizerDesc.depthClipEnable;
    dx12desc.RasterizerState.MultisampleEnable = desc.rasterizerDesc.multisampleEnable;
    dx12desc.RasterizerState.AntialiasedLineEnable = desc.rasterizerDesc.antialiasedLineEnable;

    dx12desc.BlendState.AlphaToCoverageEnable = FALSE;
    dx12desc.BlendState.IndependentBlendEnable = TRUE;
    dx12desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    dx12desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    dx12desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    dx12desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    dx12desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;//D3D11_BLEND_ONE;
    dx12desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;//D3D11_BLEND_ZERO;
    dx12desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    dx12desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    dx12desc.DSVFormat = dx12utils::ConvertToDX12DepthBufferFormat(desc.outputDesc.DSVFormat);
    dx12desc.DepthStencilState.DepthEnable = desc.outputDesc.DSVFormat == GRAPHICS_DATA_FORMAT::UNKNOWN ? FALSE : TRUE; // enable depth testing
    dx12desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // can write depth data to all of the depth/stencil buffer
    dx12desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // pixel fragment passes depth test if destination pixel's depth is less than pixel fragment's
    dx12desc.DepthStencilState.StencilEnable = FALSE; // disable stencil test
    dx12desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; // a default stencil read mask (doesn't matter at this point since stencil testing is turned off)
    dx12desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // a default stencil write mask (also doesn't matter)
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, again does not really matter since stencil testing is turned off
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    dx12desc.DepthStencilState.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
    dx12desc.DepthStencilState.BackFace = defaultStencilOp;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&dx12desc, IID_PPV_ARGS(&dx12pipeline)));

    if (desc.vs)
    {
        FileUtils::FreeBuffer(vs);
    }

    if (desc.ps)
    {
        FileUtils::FreeBuffer(ps);
    }

    if (desc.gs)
    {
        FileUtils::FreeBuffer(gs);
    }

    if (desc.hs)
    {
        FileUtils::FreeBuffer(hs);
    }

    if (desc.ds)
    {
        FileUtils::FreeBuffer(ds);
    }

	return ret;
}

void DX12Graphics::CreateShaderResourceTexture2D(
    D3D12_CPU_DESCRIPTOR_HANDLE srvGroupStart,
    uint32_t srvGroupCount,
    D3D12_CPU_DESCRIPTOR_HANDLE srv,
    const GRAPHICS_SHADER_RESOURCE_DESC& _desc, 
    SharedPtr<GraphicsShaderResource>* output
) {
    auto dx12shaderResource = std::make_shared<DX12ShaderResource>();
    dx12shaderResource->m_srv = srv;
    dx12shaderResource->m_srvGroupStart = srvGroupStart;
    dx12shaderResource->m_srvGroupCount = srvGroupCount;
    dx12shaderResource->m_desc = _desc;

    D3D12_RESOURCE_DESC texture2DDesc = {};
    texture2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture2DDesc.Width = _desc.texture2D.width;
    texture2DDesc.Height = _desc.texture2D.height;
    texture2DDesc.DepthOrArraySize = 1;
    texture2DDesc.MipLevels = _desc.texture2D.mipLevels;
    texture2DDesc.Format = dx12utils::ConvertToDX12Format(_desc.texture2D.format);
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    AllocateDX12Resource(
        &texture2DDesc,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        &dx12shaderResource->m_resource
    );

    *output = dx12shaderResource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texture2DDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture2DDesc.MipLevels;
    m_device->CreateShaderResourceView(dx12shaderResource->m_resource.resource.Get(), &srvDesc, srv);
}

void DX12Graphics::CreateShaderResources(
    uint32_t numShaderResources, 
    const GRAPHICS_SHADER_RESOURCE_DESC* descs, 
    SharedPtr<GraphicsShaderResource>* output
) {
    auto descriptorStart = GetSRVAllocator()->AllocateCPU(numShaderResources);
    auto stride = GetSRVAllocator()->GetCPUStride();

    auto* out = output;
    for (uint32_t i = 0; i < numShaderResources; i++)
    {
        auto& desc = descs[i];

        auto curDescriptor = descriptorStart;
        curDescriptor.ptr += stride * i;

        switch (desc.type)
        {
        case GRAPHICS_SHADER_RESOURCE_DESC::SHADER_RESOURCE_TYPE_TEXTURE2D:
            CreateShaderResourceTexture2D(descriptorStart, numShaderResources, curDescriptor, desc, out);
            break;
        default:
            assert(0);
            break;
        }

        out++;
    }
}

void DX12Graphics::InitImGui(void* hwnd)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ImGuiSrvDescHeap)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();

    ImFontConfig config{};
    //config.GlyphExtraSpacing.x = 1.0f;
    //config.OversampleH = config.OversampleV = 1;
    //config.PixelSnapH = true;
    //config.SizePixels = 13.0f * 1.0f;
    //config.EllipsisChar = (ImWchar)0x0085;
    //config.GlyphOffset.y = 1.0f * ((float)(int)(((config.SizePixels / 13.0f)) + 0.5f));
    config.GlyphRanges = io.Fonts->GetGlyphRangesVietnamese();
    //config.GlyphExtraSpacing.x = 1.0f;

    //io.Fonts->AddFontDefault(&config);
    io.Fonts->AddFontFromFileTTF("Resources/Default/segoeui.ttf", (int)(24.0f), &config);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(m_device.Get(), NUM_GRAPHICS_BACK_BUFFERS,
        BACK_BUFFER_FORMAT, m_ImGuiSrvDescHeap.Get(),
        m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void DX12Graphics::CreateConstantBuffers(
    uint32_t numConstantBuffers,
    const GRAPHICS_CONSTANT_BUFFER_DESC* descs,
    SharedPtr<GraphicsConstantBuffer>* output
) {
    size_t numCBVOfEachDesc = 0;
    for (size_t i = 0; i < numConstantBuffers; i++)
    {
        numCBVOfEachDesc = std::max(numCBVOfEachDesc, std::clamp(descs[i].perferNumRoom, CBV_MIN_DESCRIPTORS_PER_HEAP, CBV_MAX_DESCRIPTORS_PER_HEAP));
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = (uint32_t)numCBVOfEachDesc * numConstantBuffers;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));
    auto cpuHandleStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto cpuHandleStride = m_device->GetDescriptorHandleIncrementSize(heapDesc.Type);

    // create resource
    D3D12_GPU_VIRTUAL_ADDRESS baseGPUAddress;
    byte* uploadAddress = 0;
    {
        size_t totalSize = 0;
        size_t totalSizeOfEach = 0;
        for (size_t i = 0; i < numConstantBuffers; i++)
        {
            auto bufferSize = MemoryUtils::Align<256>(descs[i].bufferSize);
            //assert(MemoryUtils::Align<256>(bufferSize) == bufferSize);
            totalSize += bufferSize * numCBVOfEachDesc;
            totalSizeOfEach += bufferSize;
        }

        uint32_t stride = numConstantBuffers * cpuHandleStride;

        ComPtr<ID3D12Resource> resource;
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(totalSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );

        resource->Map(0, 0, (void**)&uploadAddress);

        baseGPUAddress = resource->GetGPUVirtualAddress();
        auto tempGPUAddress = baseGPUAddress;

        for (size_t i = 0; i < numConstantBuffers; i++)
        {
            auto bufferSize = MemoryUtils::Align<256>(descs[i].bufferSize);
            totalSize += bufferSize * numCBVOfEachDesc;

            auto dx12buffer = std::make_shared<DX12ConstantBuffer>();
            dx12buffer->m_descriptorHeap = descriptorHeap;
            dx12buffer->m_resource = resource;
            dx12buffer->m_baseGPUAddress = baseGPUAddress;
            dx12buffer->m_CPUHandleStride = stride;
            dx12buffer->m_numViews = numCBVOfEachDesc;
            dx12buffer->m_vaddressBytesStride = totalSizeOfEach;
            dx12buffer->m_viewIdx = 0;
            dx12buffer->m_vaddress = uploadAddress;

            dx12buffer->m_fenceValues = (uint64_t*)std::malloc(numCBVOfEachDesc * sizeof(uint64_t));
            std::memset(dx12buffer->m_fenceValues, 0, numCBVOfEachDesc * sizeof(uint64_t));

            output[i] = dx12buffer;

            baseGPUAddress += bufferSize;
            uploadAddress += bufferSize;
        }

        baseGPUAddress = tempGPUAddress;
    }
    
    // create views
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
        viewDesc.BufferLocation = baseGPUAddress;
        for (size_t j = 0; j < numCBVOfEachDesc; j++)
        {
            for (size_t i = 0; i < numConstantBuffers; i++)
            {
                auto bufferSize = MemoryUtils::Align<256>(descs[i].bufferSize);
                viewDesc.SizeInBytes = bufferSize;

                auto dx12buffer = (DX12ConstantBuffer*)output[i].get();
                if (dx12buffer->m_baseCPUHandle.ptr == 0)
                    dx12buffer->m_baseCPUHandle = cpuHandleStart;

                m_device->CreateConstantBufferView(&viewDesc, cpuHandleStart);

                cpuHandleStart.ptr += cpuHandleStride;
                viewDesc.BufferLocation += bufferSize;
            }
        }
    }

}

void DX12Graphics::InitRootSignature()
{
    constexpr size_t NUM_STAGE = 5;

    D3D12_DESCRIPTOR_RANGE ranges[2 * NUM_STAGE] = {};

    size_t descriptorHeapOffset = 0;
    size_t rangeIdx = 0;

    size_t registerSpaces[] = {
        REGISTER_SPACE_VS,
        REGISTER_SPACE_PS,
        REGISTER_SPACE_GS,
        REGISTER_SPACE_HS,
        REGISTER_SPACE_DS
    };

    size_t numCbv[] = {
        NUM_CBV_VS,
        NUM_CBV_PS,
        NUM_CBV_GS,
        NUM_CBV_HS,
        NUM_CBV_DS
    };

    size_t numSrv[] = {
        NUM_SRV_VS,
        NUM_SRV_PS,
        NUM_SRV_GS,
        NUM_SRV_HS,
        NUM_SRV_DS
    };

    D3D12_SHADER_VISIBILITY visibilities[] = {
        D3D12_SHADER_VISIBILITY_VERTEX,
        D3D12_SHADER_VISIBILITY_PIXEL,
        D3D12_SHADER_VISIBILITY_GEOMETRY,
        D3D12_SHADER_VISIBILITY_HULL,
        D3D12_SHADER_VISIBILITY_DOMAIN
    };

    D3D12_ROOT_PARAMETER rootParameters[NUM_STAGE] = {};
    for (size_t i = 0; i < NUM_STAGE; i++)
    {
        auto& parameter = rootParameters[i];
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable.pDescriptorRanges = &ranges[rangeIdx];
        parameter.DescriptorTable.NumDescriptorRanges = 2;
        parameter.ShaderVisibility = visibilities[i];

        D3D12_DESCRIPTOR_RANGE& cbvRange = ranges[rangeIdx++];
        cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        cbvRange.BaseShaderRegister = 0;
        cbvRange.RegisterSpace = registerSpaces[i];
        cbvRange.NumDescriptors = numCbv[i];
        cbvRange.OffsetInDescriptorsFromTableStart = descriptorHeapOffset;

        descriptorHeapOffset += cbvRange.NumDescriptors;

        D3D12_DESCRIPTOR_RANGE& srvRange = ranges[rangeIdx++];
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = registerSpaces[i];
        srvRange.NumDescriptors = numSrv[i];
        srvRange.OffsetInDescriptorsFromTableStart = descriptorHeapOffset;

        descriptorHeapOffset += srvRange.NumDescriptors;

        descriptorHeapOffset = 0;
    }

    D3D12_STATIC_SAMPLER_DESC rootSampler = {};
    rootSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    rootSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;//D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    rootSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    rootSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    rootSampler.MipLODBias = 0;
    rootSampler.MaxAnisotropy = 0;
    rootSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    rootSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    rootSampler.MinLOD = 0.0f;
    rootSampler.MaxLOD = D3D12_FLOAT32_MAX;
    rootSampler.ShaderRegister = 0;
    rootSampler.RegisterSpace = 0;
    rootSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = NUM_STAGE;
    rootSignatureDesc.pStaticSamplers = &rootSampler;
    rootSignatureDesc.NumStaticSamplers = 1;

    ComPtr<ID3DBlob> signature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr));

    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));
}

void DX12Graphics::InitGPUVisibleDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC gpuHeapDesc = {};
    gpuHeapDesc.NumDescriptors = TOTAL_DESCRIPTORS_OF_GPU_VISIBLE_HEAP;
    gpuHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    gpuHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&gpuHeapDesc, IID_PPV_ARGS(&m_gpuVisibleHeap)));

    m_gpuVisibleHeapCPUHandleStart = m_gpuVisibleHeap->GetCPUDescriptorHandleForHeapStart();
    m_gpuVisibleHeapGPUHandleStart = m_gpuVisibleHeap->GetGPUDescriptorHandleForHeapStart();
}

void DX12Graphics::StageCurrentRenderParams()
{
    // set up render pipeline params
    auto cmdList = GetCmdList();
    auto paramsRoom = m_currentGraphicsPipeline->GetCurrentRenderParamsRoom();
    auto cpuDescriptorHandleCBV = GetCurrentRenderRoomCPUDescriptorHandleStart();
    auto gpuDescriptorHandle = GetCurrentRenderRoomGPUDescriptorHandleStart();
    auto cpuDescriptorHandleSRV = cpuDescriptorHandleCBV;
    cpuDescriptorHandleSRV.ptr += GRAPHICS_PARAMS_DESC::NUM_CONSTANT_BUFFER * GetCbvSrvUavCPUDescriptorHandleStride();

    auto spaceStride = (GRAPHICS_PARAMS_DESC::NUM_CONSTANT_BUFFER + GRAPHICS_PARAMS_DESC::NUM_SHADER_RESOURCE)
        * GetCbvSrvUavCPUDescriptorHandleStride();

    uint32_t stageIndex = 0;
    for (auto& paramsShaderSpace : paramsRoom->m_params)
    {
        cmdList->SetGraphicsRootDescriptorTable(stageIndex, gpuDescriptorHandle);

//#ifdef _DEBUG
//        auto start = GetCurrentRenderRoomCPUDescriptorHandleStart();
//        start.ptr += spaceStride * stageIndex;
//        assert(start.ptr == cpuDescriptorHandleCBV.ptr);
//        assert(cpuDescriptorHandleCBV.ptr + GRAPHICS_PARAMS_DESC::NUM_CONSTANT_BUFFER * GetCbvSrvUavCPUDescriptorHandleStride()
//            == cpuDescriptorHandleSRV.ptr);
//#endif // _DEBUG


        if (paramsShaderSpace.m_constantBufferDescriptorRangesIdx != 0)
        {
            // copy all descriptor ranges to shader visible descriptor heap
            for (uint32_t i = 0; i < paramsShaderSpace.m_constantBufferDescriptorRangesIdx; i++)
            {
                auto& range = paramsShaderSpace.m_constantBufferDescriptorRanges[i];
                auto registerHandle = cpuDescriptorHandleCBV;
                registerHandle.ptr += range.baseRegisterIndex * GetCbvSrvUavCPUDescriptorHandleStride();
                m_device->CopyDescriptorsSimple(range.count, registerHandle, range.baseCPUDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }

        if (paramsShaderSpace.m_shaderResourceDescriptorRangesIdx != 0)
        {
            // copy all descriptor ranges to shader visible descriptor heap
            for (uint32_t i = 0; i < paramsShaderSpace.m_shaderResourceDescriptorRangesIdx; i++)
            {
                auto& range = paramsShaderSpace.m_shaderResourceDescriptorRanges[i];
                auto registerHandle = cpuDescriptorHandleSRV;
                registerHandle.ptr += range.baseRegisterIndex * GetCbvSrvUavCPUDescriptorHandleStride();
                m_device->CopyDescriptorsSimple(range.count, registerHandle, range.baseCPUDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }

        cpuDescriptorHandleCBV.ptr += spaceStride;
        cpuDescriptorHandleSRV.ptr += spaceStride;
        stageIndex++;
        gpuDescriptorHandle.ptr += spaceStride;
    }

    m_currentGraphicsPipeline->m_lastFenceValue = GetCurrentDX12FenceValue();
}

void DX12Graphics::CreateRenderTargets(
    uint32_t numRenderTargets,
    const GRAPHICS_RENDER_TARGET_DESC* _desc,
    SharedPtr<GraphicsRenderTarget>* output
) {
    auto rtvStart = GetRTVAllocator()->AllocateCPU(numRenderTargets);
    auto rtSRVStart = GetSRVAllocator()->AllocateCPU(numRenderTargets);

    auto curRTV = rtvStart;
    auto curRTVSRV = rtSRVStart;

    for (uint32_t i = 0; i < numRenderTargets; i++)
    {
        auto rt = std::make_shared<DX12RenderTarget>();
        auto shaderResource = std::make_shared<DX12ShaderResource>();

        auto& desc = _desc[i];

        D3D12_RESOURCE_DESC texture2DDesc = {};
        texture2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texture2DDesc.Width = desc.width == -1 ? m_windowWidth : desc.width;
        texture2DDesc.Height = desc.height == -1 ? m_windowHeight : desc.height;
        texture2DDesc.DepthOrArraySize = 1;
        texture2DDesc.MipLevels = desc.mipLevels;
        texture2DDesc.Format = dx12utils::ConvertToDX12Format(desc.format);
        texture2DDesc.SampleDesc.Count = 1;
        texture2DDesc.SampleDesc.Quality = 0;
        texture2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texture2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        rt->m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = texture2DDesc.Format;
        std::memcpy(clearValue.Color, &desc.clearColor, sizeof(Vec4));

        AllocateDX12Resource(
            &texture2DDesc,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            &rt->m_dx12Resource
        );

        auto dx12resource = rt->m_dx12Resource.resource.Get();

        m_device->CreateRenderTargetView(dx12resource, 0, curRTV);
        rt->m_rtv = curRTV;

        m_device->CreateShaderResourceView(dx12resource, 0, curRTVSRV);
        shaderResource->m_srvGroupStart = rtSRVStart;
        shaderResource->m_srvGroupCount = numRenderTargets;
        shaderResource->m_srv = curRTVSRV;
        shaderResource->m_resource = rt->m_dx12Resource;

        auto& srvDesc = shaderResource->m_desc;
        srvDesc.type = GRAPHICS_SHADER_RESOURCE_DESC::SHADER_RESOURCE_TYPE_TEXTURE2D;
        srvDesc.texture2D.format = desc.format;
        srvDesc.texture2D.mipLevels = desc.mipLevels;
        srvDesc.texture2D.width = texture2DDesc.Width;
        srvDesc.texture2D.height = texture2DDesc.Height;

        rt->m_shaderResource = shaderResource;

        curRTV.ptr += GetRTVAllocator()->GetCPUStride();
        curRTVSRV.ptr += GetSRVAllocator()->GetCPUStride();

        output[i] = rt;
    }
}

SharedPtr<GraphicsDepthStencilBuffer> DX12Graphics::CreateDepthStencilBuffer(const GRAPHICS_DEPTH_STENCIL_BUFFER_DESC& desc)
{
    auto ds = std::make_shared<DX12DepthStencilBuffer>();
    auto shaderResource = std::make_shared<DX12ShaderResource>();

    D3D12_RESOURCE_DESC texture2DDesc = {};
    texture2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture2DDesc.Width = desc.width == -1 ? m_windowWidth : desc.width;
    texture2DDesc.Height = desc.height == -1 ? m_windowHeight : desc.height;
    texture2DDesc.DepthOrArraySize = 1;
    texture2DDesc.MipLevels = desc.mipLevels;
    texture2DDesc.Format = dx12utils::ConvertToDX12DepthBufferFormat(desc.format);
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    ds->m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = texture2DDesc.Format;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    AllocateDX12Resource(
        &texture2DDesc,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        &ds->m_dx12Resource
    );

    auto dx12resource = ds->m_dx12Resource.resource.Get();

    auto dsvStart = GetDSVAllocator()->AllocateCPU(1);
    m_device->CreateDepthStencilView(dx12resource, 0, dsvStart);
    ds->m_dsv = dsvStart;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvView = {};
    srvView.Format = dx12utils::ConvertToDX12Format(desc.format);
    srvView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvView.Texture2D.MipLevels = 1;

    auto dsSRVStart = GetSRVAllocator()->AllocateCPU(1);
    m_device->CreateShaderResourceView(dx12resource, &srvView, dsSRVStart);
    shaderResource->m_srvGroupStart = dsSRVStart;
    shaderResource->m_srvGroupCount = 1;
    shaderResource->m_srv = dsSRVStart;
    shaderResource->m_resource = ds->m_dx12Resource;

    ds->m_shaderResource = shaderResource;

    auto& srvDesc = shaderResource->m_desc;
    srvDesc.type = GRAPHICS_SHADER_RESOURCE_DESC::SHADER_RESOURCE_TYPE_TEXTURE2D;
    srvDesc.texture2D.format = desc.format;
    srvDesc.texture2D.mipLevels = desc.mipLevels;
    srvDesc.texture2D.width = texture2DDesc.Width;
    srvDesc.texture2D.height = texture2DDesc.Height;
    return ds;
}

SharedPtr<GraphicsVertexBuffer> DX12Graphics::CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc)
{
    auto vertexBuffer = std::make_shared<DX12VertexBuffer>();
    vertexBuffer->m_view.SizeInBytes = desc.stride * desc.count;
    vertexBuffer->m_view.StrideInBytes = desc.stride;

    D3D12_RESOURCE_DESC vbufDesc = {};
    vbufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbufDesc.Width = desc.stride * desc.count;
    vbufDesc.Height = 1;
    vbufDesc.DepthOrArraySize = 1;
    vbufDesc.MipLevels = 1;
    vbufDesc.SampleDesc.Count = 1;
    vbufDesc.SampleDesc.Quality = 0;
    vbufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    AllocateDX12Resource(
        &vbufDesc,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        &vertexBuffer->m_resource
    );

    vertexBuffer->m_view.BufferLocation = vertexBuffer->m_resource.resource->GetGPUVirtualAddress();
    
	return vertexBuffer;
}

SharedPtr<GraphicsIndexBuffer> DX12Graphics::CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc)
{
	return SharedPtr<GraphicsIndexBuffer>();
}

void DX12Graphics::SetGraphicsPipeline(GraphicsPipeline* graphicsPipeline)
{
    auto dx12pipeline = (DX12GraphicsPipeline*)graphicsPipeline;
    GetCmdList()->SetPipelineState(dx12pipeline->m_pipelineState.Get());
    m_currentGraphicsPipeline = dx12pipeline;
    m_currentDX12GraphicsPipeline = dx12pipeline->m_pipelineState;

    m_currentGraphicsPipeline->m_lastFenceValue = GetCurrentDX12FenceValue();
}

void DX12Graphics::SetDrawParams(GraphicsParams* params)
{
}

void DX12Graphics::SetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv)
{
    auto dx12dsv = (DX12DepthStencilBuffer*)dsv;

    auto& RTVs = m_currentBoundRTVs;

    auto cmdList = GetCmdList();

    D3D12_RESOURCE_BARRIER barrier = {};
    for (uint32_t i = 0; i < numRT; i++)
    {
        auto dx12rtv = (DX12RenderTarget*)rtv[i];
        RTVs[i] = dx12rtv->m_rtv;

        if (dx12rtv->m_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            barrier.Transition.pResource = dx12rtv->m_dx12Resource.resource.Get();
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.StateBefore = dx12rtv->m_currentState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            cmdList->ResourceBarrier(1, &barrier);
            dx12rtv->m_currentState = barrier.Transition.StateAfter;
        }

#ifdef _DEBUG
        if (m_currentRTs[i])
        {
            assert(0 && "SetRenderTargets() and UnsetRenderTargets() missmatch");
        }

        m_currentRTs[i] = dx12rtv;
#endif // _DEBUG
    }

#ifdef _DEBUG
    if (m_numCurrentRT || m_currentDS)
    {
        assert(0 && "SetRenderTargets() and UnsetRenderTargets() missmatch");
    }

    m_numCurrentRT = numRT;
    m_currentDS = dx12dsv;
#endif // _DEBUG

    if (dsv)
    {
        auto dx12DSV = (DX12DepthStencilBuffer*)dsv;

        if (dx12DSV->m_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            barrier.Transition.pResource = dx12DSV->m_dx12Resource.resource.Get();
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.StateBefore = dx12DSV->m_currentState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            cmdList->ResourceBarrier(1, &barrier);
            dx12DSV->m_currentState = barrier.Transition.StateAfter;
        }
        
        cmdList->OMSetRenderTargets(numRT, RTVs, 0, &dx12dsv->m_dsv);

        m_currentBoundDSV = dx12dsv->m_dsv;
    }
    else
    {
        cmdList->OMSetRenderTargets(numRT, RTVs, 0, 0);
    }

    cmdList->RSSetViewports(1, &m_backBufferViewport); // set the viewports
    cmdList->RSSetScissorRects(1, &m_backBufferScissorRect); // set the scissor rects

    m_numBoundRTVs = numRT;
}

void DX12Graphics::UnsetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv)
{
    auto cmdList = GetCmdList();

    D3D12_RESOURCE_BARRIER barrier = {};

    for (uint32_t i = 0; i < numRT; i++)
    {
        auto dx12rtv = (DX12RenderTarget*)rtv[i];
        dx12rtv->m_fenceValue = GetCurrentDX12FenceValue();

        if (dx12rtv->m_currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        {
            barrier.Transition.pResource = dx12rtv->m_dx12Resource.resource.Get();
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.StateBefore = dx12rtv->m_currentState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            cmdList->ResourceBarrier(1, &barrier);
            dx12rtv->m_currentState = barrier.Transition.StateAfter;
        }
    }

    if (dsv)
    {
        auto dx12DSV = (DX12DepthStencilBuffer*)dsv;
        dx12DSV->m_fenceValue = GetCurrentDX12FenceValue();

        if (dx12DSV->m_currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        {
            barrier.Transition.pResource = dx12DSV->m_dx12Resource.resource.Get();
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.StateBefore = dx12DSV->m_currentState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            cmdList->ResourceBarrier(1, &barrier);
            dx12DSV->m_currentState = barrier.Transition.StateAfter;
        }
    }

#ifdef _DEBUG
    assert(numRT == m_numCurrentRT);
    for (uint32_t i = 0; i < numRT; i++)
    {
        assert(m_currentRTs[i] == rtv[i]);
        m_currentRTs[i] = 0;
    }
    m_numCurrentRT = 0;
    assert(m_currentDS == dsv);
    m_currentDS = 0;
#endif // _DEBUG

    m_numBoundRTVs = 0;
    m_currentBoundDSV.ptr = 0;
}

void DX12Graphics::DrawInstanced(uint32_t numVertexBuffers, GraphicsVertexBuffer** vertexBuffers, 
    uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    m_renderRoomIdx = (m_renderRoomIdx + 1) % NUM_RENDER_ROOM;
    WaitForDX12FenceValue(m_renderRoomFenceValues[m_renderRoomIdx]);
    m_renderRoomFenceValues[m_renderRoomIdx] = GetCurrentDX12FenceValue();
    //std::cout << m_renderRoomIdx << '\n';

    auto cmdList = GetCmdList();

    StageCurrentRenderParams();

    // set up vertex views
    if (numVertexBuffers)
    {
        for (uint32_t i = 0; i < numVertexBuffers; i++)
        {
            auto dx12vertexBuffer = (DX12VertexBuffer*)vertexBuffers[i];
            dx12vertexBuffer->m_lastFenceValue = GetCurrentDX12FenceValue();
            m_vertexBufferViews[i] = dx12vertexBuffer->m_view;
        }
        cmdList->IASetVertexBuffers(0, numVertexBuffers, m_vertexBufferViews);
    }

    cmdList->IASetPrimitiveTopology(m_currentGraphicsPipeline->m_primitiveTopology);
    cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);

    if ((++m_renderCallCount) % NUM_RENDER_CALL_PER_DISPATCH_CMD_LIST == 0)
    {
        //std::cout << "Dispatched\n";
        ExecuteCurrentCmdList();
    }
}

void DX12Graphics::DrawIndexedInstanced(GraphicsIndexBuffer* indexBuffer, uint32_t indexCountPerInstance, uint32_t numVertexBuffers, GraphicsVertexBuffer** vertexBuffers, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
}

void DX12Graphics::ClearRenderTarget(GraphicsRenderTarget* rtv, Vec4 clearColor, uint32_t numRect, const AARect* rects)
{
    auto dx12rtv = (DX12RenderTarget*)rtv;
    auto cmdList = GetCmdList();
    cmdList->ClearRenderTargetView(dx12rtv->m_rtv, &clearColor[0], 0, 0);
}

void DX12Graphics::ClearDepthStencil(GraphicsDepthStencilBuffer* dsv, uint32_t numRect, const AARect* rects)
{
    auto dx12dsv = (DX12DepthStencilBuffer*)dsv;
    auto cmdList = GetCmdList();
    cmdList->ClearDepthStencilView(dx12dsv->m_dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, 0);
}

void DX12Graphics::AllocateDX12Resource(
    const D3D12_RESOURCE_DESC* desc, 
    D3D12_HEAP_TYPE heapType, 
    D3D12_RESOURCE_STATES resourceState,
    const D3D12_CLEAR_VALUE* clearValue,
    DX12Resource* output
) {
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = heapType;

    ThrowIfFailed(m_dx12ResourceAllocator->CreateResource(
        &allocationDesc,
        desc,
        resourceState,
        clearValue,
        &output->allocation,
        IID_PPV_ARGS(&output->resource)
    ));

    //D3D12_HEAP_PROPERTIES heapProps = {};
    //heapProps.Type = heapType;
   
    //ThrowIfFailed(
    //    m_device->CreateCommittedResource(
    //        &heapProps, // a default heap
    //        D3D12_HEAP_FLAG_NONE, // no flags
    //        desc,
    //        resourceState,
    //        clearValue,
    //        IID_PPV_ARGS(&output->resource)
    //    )
    //);
}

GraphicsRenderTarget* DX12Graphics::GetScreenRenderTarget()
{
    return &m_currentRenderTarget;
}

GraphicsDepthStencilBuffer* DX12Graphics::GetScreenDepthStencilBuffer()
{
    return &m_currentDepthStencilBuffer;
}

uint64_t DX12Graphics::GetCurrentFenceValue()
{
    return 0;
}

void DX12Graphics::WaitForFenceValue(uint64_t value)
{
}

void DX12Graphics::BeginFrame()
{
    m_frameCount++;

    m_currentRenderTarget.m_dx12Resource.resource = m_renderTargets[m_currentBackBufferId];
    m_currentRenderTarget.m_rtv.ptr = m_rtvStart.ptr + m_currentBackBufferId * m_rtvAllocator.GetCPUStride();
    auto rtSRVStart = m_currentRenderTarget.GetDX12ShaderResource()->m_srvGroupStart;
    m_currentRenderTarget.GetDX12ShaderResource()->m_srv.ptr = rtSRVStart.ptr + m_currentBackBufferId * GetSRVAllocator()->GetCPUStride();

    m_currentDepthStencilBuffer.m_dsv.ptr = m_dsvStart.ptr + m_currentBackBufferId * m_dsvAllocator.GetCPUStride();
    auto dsSRVStart = m_currentDepthStencilBuffer.GetDX12ShaderResource()->m_srvGroupStart;
    m_currentDepthStencilBuffer.GetDX12ShaderResource()->m_srv.ptr = dsSRVStart.ptr + m_currentBackBufferId * GetSRVAllocator()->GetCPUStride();
    m_currentDepthStencilBuffer.m_dx12Resource.resource = m_depthBuffers[m_currentBackBufferId].resource;

    WaitForDX12FenceValue(m_frameFenceValues[m_currentBackBufferId]);

    //auto cmdList = GetCmdList();

    /*D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Transition.pResource = m_currentRenderTarget.m_dx12Resource.resource.Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cmdList->ResourceBarrier(1, &barrier);*/

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DX12Graphics::EndFrame()
{
    auto cmdList = GetCmdList();
    ExecuteCurrentCmdList();

    D3D12_RESOURCE_BARRIER barrier = {};

    {
        auto screenRT = GetScreenRenderTarget();
        SetRenderTargets(1, &screenRT, nullptr);

        // Rendering
        ImGui::Render();

        ID3D12DescriptorHeap* heaps[] = { m_ImGuiSrvDescHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

        UnsetRenderTargets(1, &screenRT, nullptr);
    }

    if (m_currentRenderTarget.m_currentState != D3D12_RESOURCE_STATE_PRESENT)
    {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_renderTargets[m_currentBackBufferId].Get();
        barrier.Transition.StateBefore = m_currentRenderTarget.m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrier);
        m_currentRenderTarget.m_currentState = barrier.Transition.StateAfter;
    }

    if (m_currentDepthStencilBuffer.m_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_depthBuffers[m_currentBackBufferId].resource.Get();
        barrier.Transition.StateBefore = m_currentDepthStencilBuffer.m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        cmdList->ResourceBarrier(1, &barrier);
        m_currentDepthStencilBuffer.m_currentState = barrier.Transition.StateAfter;
    }

    ExecuteCurrentCmdList();

    m_frameFenceValues[m_currentBackBufferId] = GetCurrentDX12FenceValue() - 1;
    /*ThrowIfFailed(
        m_commandQueue->Signal(
            m_fence.Get(),
            m_currentFenceValue++
        )
    );*/

    m_currentBackBufferId = (m_currentBackBufferId + 1) % NUM_GRAPHICS_BACK_BUFFERS;

    m_currentGraphicsPipeline = nullptr;
    m_currentDX12GraphicsPipeline = nullptr;
    ProcessFreeDX12ResourceList();
}

void DX12Graphics::Present(bool vsync)
{
    ThrowIfFailed(m_swapChain->Present(vsync ? 1 : 0, 0));
}

NAMESPACE_DX12_END