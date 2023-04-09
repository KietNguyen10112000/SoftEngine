#include "DX12Graphics.h"
#include <cassert>

NAMESPACE_DX12_BEGIN

DX12Graphics::DX12Graphics(void* _hwnd)
{
    InitD3D12();
    InitSwapchain(_hwnd);
    InitCommandLists();
    InitRootSignature();
    InitDescriptorHeap();
    SetInitState();
}

DX12Graphics::~DX12Graphics()
{
    m_graphicsCommandList.Destroy();
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
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;
    swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ThrowIfFailed(m_dxgiFactory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &tempSwapChain));
    ThrowIfFailed(tempSwapChain.As(&m_swapChain));
    m_currentBackBufferId = m_swapChain->GetCurrentBackBufferIndex();


    // create rt for each back buffer
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = NUM_GRAPHICS_BACK_BUFFERS;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));

    auto gpuRTVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto gpuRTVCPUHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    m_cpuRTVDescriptorSize = gpuRTVDescriptorSize;
    m_cpuRTVDescriptorHandle = gpuRTVCPUHandle;

    for (size_t i = 0; i < NUM_GRAPHICS_BACK_BUFFERS; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), 0, gpuRTVCPUHandle);
        gpuRTVCPUHandle.ptr += gpuRTVDescriptorSize;
    }


    // create depth buffer for each back buffer
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = NUM_GRAPHICS_BACK_BUFFERS;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap)));

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

    auto cpuDSVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    auto cpuDSVHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    m_cpuDSVDescriptorSize = cpuDSVDescriptorSize;
    m_cpuDSVDescriptorHandle = cpuDSVHandle;

    for (size_t i = 0; i < NUM_GRAPHICS_BACK_BUFFERS; i++)
    {
        ThrowIfFailed(
            m_device->CreateCommittedResource(
                &heapProps, // a default heap
                D3D12_HEAP_FLAG_NONE, // no flags
                &depthBufferDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                IID_PPV_ARGS(&m_depthBuffers[i])
            )
        );

        m_device->CreateDepthStencilView(m_depthBuffers[i].Get(), &dsvDesc, cpuDSVHandle);
        cpuDSVHandle.ptr += cpuDSVDescriptorSize;
    }
}

void DX12Graphics::InitCommandLists()
{
    m_graphicsCommandList.Init(m_device.Get());
    m_userCmdList.Init(this);

    m_fenceEvent    = m_graphicsCommandList.m_fenceEvent;
    m_fenceValue    = &m_graphicsCommandList.m_fenceValue;
    m_fence         = m_graphicsCommandList.m_fence.Get();
}

void DX12Graphics::InitRootSignature()
{
    D3D12_DESCRIPTOR_RANGE ranges[2] = {};

    D3D12_DESCRIPTOR_RANGE& cbvRange = ranges[0];
    cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvRange.BaseShaderRegister = 0;
    cbvRange.RegisterSpace = 0;
    cbvRange.NumDescriptors = NUM_CBV_PER_PSO;
    cbvRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_DESCRIPTOR_RANGE& srvRange = ranges[1];
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.NumDescriptors = NUM_SRV_PER_PSO;
    srvRange.OffsetInDescriptorsFromTableStart = NUM_CBV_PER_PSO;

    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter.DescriptorTable.pDescriptorRanges = ranges;
    rootParameter.DescriptorTable.NumDescriptorRanges = ARRAYSIZE(ranges);
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC rootSampler = {};
    rootSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    rootSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    rootSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    rootSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
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
    rootSignatureDesc.pParameters = &rootParameter;
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pStaticSamplers = &rootSampler;
    rootSignatureDesc.NumStaticSamplers = 1;

    ComPtr<ID3DBlob> signature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr));

    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));
}

void DX12Graphics::InitDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = GPU_DESCRIPTOR_HEAP_SIZE;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_gpuDescriptorHeap)));
}

void DX12Graphics::SetInitState()
{
    auto dx12CmdList = m_graphicsCommandList.m_commandList.Get();
    m_graphicsCommandList.BeginCommandList(m_commandQueue.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_gpuDescriptorHeap.Get() };
    dx12CmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    dx12CmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    m_graphicsCommandList.EndCommandList(m_commandQueue.Get());
}

void DX12Graphics::BeginCommandList(GraphicsCommandList** cmdList)
{
    *cmdList = &m_userCmdList;
    m_graphicsCommandList.Reset();
}

void DX12Graphics::EndCommandList(GraphicsCommandList** cmdList)
{
    *cmdList = nullptr;
    m_graphicsCommandList.EndCommandList(m_commandQueue.Get());
}

void DX12Graphics::BeginFrame()
{
    m_currentBackBufferId = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Graphics::EndFrame()
{
    m_graphicsCommandList.Reset();
    auto dx12CmdList = m_graphicsCommandList.m_commandList.Get();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_renderTargets[m_currentBackBufferId].Get();
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    dx12CmdList->ResourceBarrier(1, &barrier);

    m_graphicsCommandList.EndCommandList(m_commandQueue.Get());

    // 2th param can be DXGI_PRESENT_ALLOW_TEARING
    m_swapChain->Present(1, 0);
}

NAMESPACE_DX12_END