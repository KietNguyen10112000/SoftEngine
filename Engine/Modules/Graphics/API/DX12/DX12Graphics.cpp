#include "DX12Graphics.h"
#include <cassert>

#include "Components/Rendering/Camera.h"

#include "DX12DebugGraphics.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#define CPP
#include "Shaders/TypeDef.hlsli"

NAMESPACE_DX12_BEGIN

DX12Graphics::DX12Graphics(void* _hwnd)
{
    InitD3D12();
    InitSwapchain(_hwnd);
    InitCommandLists();
    InitRenderRooms();
    InitBuiltInParams();
    InitImGui(_hwnd);
}

DX12Graphics::~DX12Graphics()
{
    m_renderRooms.Destroy();
    m_graphicsCommandList.Destroy();

    // ImGui Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_sceneParams.Destroy();
    m_cameraParams.Destroy();
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

    m_backBufferViewport.TopLeftX   = 0;
    m_backBufferViewport.TopLeftY   = 0;
    m_backBufferViewport.MinDepth   = 0.0f;
    m_backBufferViewport.MaxDepth   = 1.0f;
    m_backBufferViewport.Width      = WINDOW_WIDTH;
    m_backBufferViewport.Height     = WINDOW_HEIGHT;

    m_backBufferScissorRect.top     = 0;
    m_backBufferScissorRect.left    = 0;
    m_backBufferScissorRect.right   = WINDOW_WIDTH;
    m_backBufferScissorRect.bottom  = WINDOW_HEIGHT;

    //m_swapChain->SetFullscreenState(true, nullptr);
}

void DX12Graphics::InitCommandLists()
{
    m_graphicsCommandList.Init(m_device.Get());
    m_userCmdList.Init(this);

    m_synchObject.fenceEvent    = m_graphicsCommandList.m_fenceEvent;
    m_synchObject.fenceValue    = &m_graphicsCommandList.m_fenceValue;
    m_synchObject.fence         = m_graphicsCommandList.m_fence.Get();

    m_commandList = m_graphicsCommandList.m_commandList.Get();
}

void DX12Graphics::InitRenderRooms()
{
    m_renderRooms.Init(NUM_RENDER_ROOMS, m_device.Get(), &m_synchObject);
}

void DX12Graphics::InitBuiltInParams()
{
    {
        size_t constBufSizes[] = {
            MemoryUtils::Align<256>(sizeof(SceneData)),
        };

        m_sceneParams.Init(m_device.Get(), 1, 3, 0, 1, constBufSizes, 0, 0, 0, &m_synchObject);
        m_builtInCBVs[0] = m_sceneParams.m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    }

    {
        size_t constBufSizes[] = {
            MemoryUtils::Align<256>(sizeof(CameraData)),
        };

        m_cameraParams.Init(m_device.Get(), 1, 16, 1, 1, constBufSizes, 0, 0, 0, &m_synchObject);
        m_builtInCBVs[1] = m_cameraParams.m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
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

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(m_device.Get(), NUM_GRAPHICS_BACK_BUFFERS,
        BACK_BUFFER_FORMAT, m_ImGuiSrvDescHeap.Get(),
        m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void DX12Graphics::BeginCommandList()
{
    m_currentBackBufferId = m_swapChain->GetCurrentBackBufferIndex();

    m_graphicsCommandList.Reset();

    m_renderRooms.BeginCommandList(m_commandList);

    auto currentRTV = m_cpuRTVDescriptorHandle;
    currentRTV.ptr += m_currentBackBufferId * m_cpuRTVDescriptorSize;
    auto currentDSV = m_cpuDSVDescriptorHandle;
    currentDSV.ptr += m_currentBackBufferId * m_cpuDSVDescriptorSize;
    m_commandList->OMSetRenderTargets(1, &currentRTV, 0, &currentDSV);

    m_commandList->RSSetViewports(1, &m_backBufferViewport);
    m_commandList->RSSetScissorRects(1, &m_backBufferScissorRect);
}

void DX12Graphics::EndCommandList()
{
    m_graphicsCommandList.EndCommandList(m_commandQueue.Get());
}

void DX12Graphics::BeginFrame(GraphicsCommandList** cmdList)
{
    *cmdList = &m_userCmdList;
    BeginCommandList();

    static float t = 0;
    auto dt = GetRenderingSystem()->GetScene()->Dt();
    t += dt;

    {
        auto head = m_sceneParams.AllocateConstantBufferWriteHead();
        auto scene = (SceneData*)m_sceneParams.GetConstantBuffer(head, 0);
        scene->t = t;
        scene->dt = dt;
        m_sceneParams.BeginRenderingAsBuiltInParam(m_commandList, m_builtInCBVs, 0);
    }

    ((DX12DebugGraphics*)m_debugGraphics)->BeginFrame();
}

void DX12Graphics::EndFrame(GraphicsCommandList** cmdList)
{
    *cmdList = nullptr;

    auto dx12CmdList = m_commandList;

    m_sceneParams.EndRenderingAsBuiltInParam(m_commandQueue.Get());

    ((DX12DebugGraphics*)m_debugGraphics)->EndFrame();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_renderTargets[m_currentBackBufferId].Get();
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    dx12CmdList->ResourceBarrier(1, &barrier);

    EndCommandList();

    // 2th param can be DXGI_PRESENT_ALLOW_TEARING
    m_swapChain->Present(1, 0);
}

void DX12Graphics::BeginCamera(Camera* camera)
{
    auto head = m_cameraParams.AllocateConstantBufferWriteHead();
    auto camData = (CameraData*)m_cameraParams.GetConstantBuffer(head, 1);

    auto view = camera->GetView();
    auto& proj = camera->GetProj();

    camData->transform = camera->GetObject()->GetTransformMat4();
    camData->view = view;
    camData->proj = proj;
    camData->vp = view * proj;

    /*camData->vp = 
        Mat4::Identity().SetLookAtLH({ 0, 0, 10 }, { 0,0,0 }, Vec3::UP)
        * Mat4::Identity().SetPerspectiveFovLH(PI / 3.0f, 16 / 9.0f, 0.01f, 1000);*/

    m_cameraParams.BeginRenderingAsBuiltInParam(m_commandList, m_builtInCBVs + 1, 0);

    ((DX12DebugGraphics*)m_debugGraphics)->BeginCamera();
}

void DX12Graphics::EndCamera(Camera* camera)
{
    ((DX12DebugGraphics*)m_debugGraphics)->EndCamera();
    m_cameraParams.EndRenderingAsBuiltInParam(m_commandQueue.Get());
}

void DX12Graphics::BeginGUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DX12Graphics::EndGUI()
{
    auto dx12CmdList = m_commandList;

    // Rendering
    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { m_ImGuiSrvDescHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12CmdList);

    EndCommandList();
    BeginCommandList();
}

NAMESPACE_DX12_END