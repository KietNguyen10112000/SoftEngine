#include "DX12Graphics.h"

#include "DX12DescriptorAllocator.h"
#include "DX12Utils.h"
#include "DX12Resources.h"

#include "FileSystem/FileUtils.h"

#include "Runtime/StartupConfig.h"


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

    m_compiledShadersPath = StartupConfig::Get().compiledShadersPath;
}

DX12Graphics::~DX12Graphics()
{
    SignalCurrentDX12FenceValue();
    if (m_currentFenceValue != 0)
    {
        WaitForDX12FenceValue(m_currentFenceValue - 1);
    }
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


    // create rt for each back buffer
    auto cpuHandle = m_rtvAllocator.AllocateCPU(NUM_GRAPHICS_BACK_BUFFERS);
    auto stride = m_rtvAllocator.GetCPUStride();
    m_rtvStart = cpuHandle;
    for (size_t i = 0; i < NUM_GRAPHICS_BACK_BUFFERS; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), 0, cpuHandle);
        cpuHandle.ptr += stride;
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
    }

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

    dx12desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    dx12desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    dx12desc.RasterizerState.FrontCounterClockwise = FALSE;
    dx12desc.RasterizerState.DepthBias = 0;
    dx12desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
    dx12desc.RasterizerState.DepthBiasClamp = 0.0f;
    dx12desc.RasterizerState.DepthClipEnable = TRUE;
    dx12desc.RasterizerState.MultisampleEnable = FALSE;
    dx12desc.RasterizerState.AntialiasedLineEnable = FALSE;

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

    dx12desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    dx12desc.DepthStencilState.DepthEnable = TRUE; // enable depth testing
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

void DX12Graphics::CreateShaderResources(
    uint32_t numShaderResources, 
    const GRAPHICS_SHADER_RESOURCE_DESC* descs, 
    SharedPtr<GraphicsShaderResource>* output
) {
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
    }

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

SharedPtr<GraphicsRenderTarget> DX12Graphics::CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc)
{
	return SharedPtr<GraphicsRenderTarget>();
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
}

void DX12Graphics::SetDrawParams(GraphicsParams* params)
{
}

void DX12Graphics::SetRenderTarget(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv)
{
    
    auto dx12dsv = (DX12DepthStencilBuffer*)dsv;

    D3D12_CPU_DESCRIPTOR_HANDLE RTVs[8] = {};
    for (uint32_t i = 0; i < numRT; i++)
    {
        auto dx12rtv = (DX12RenderTarget*)rtv[i];
        RTVs[i] = dx12rtv->m_rtv;
    }

    auto cmdList = GetCmdList();
    cmdList->OMSetRenderTargets(numRT, RTVs, 0, &dx12dsv->m_dsv);

    cmdList->RSSetViewports(1, &m_backBufferViewport); // set the viewports
    cmdList->RSSetScissorRects(1, &m_backBufferScissorRect); // set the scissor rects
}

void DX12Graphics::DrawInstanced(uint32_t numVertexBuffers, GraphicsVertexBuffer** vertexBuffers, 
    uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    WaitForDX12FenceValue(m_renderRoomFenceValues[m_renderRoomIdx]);
    m_renderRoomFenceValues[m_renderRoomIdx] = GetCurrentDX12FenceValue();

    auto cmdList = GetCmdList();

    // set up render pipeline params
    {
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
    }

    // set up vertex views
    {
        for (uint32_t i = 0; i < numVertexBuffers; i++)
        {
            m_vertexBufferViews[i] = ((DX12VertexBuffer*)vertexBuffers[i])->m_view;
        }
    }

    cmdList->IASetPrimitiveTopology(m_currentGraphicsPipeline->m_primitiveTopology);
    cmdList->IASetVertexBuffers(0, numVertexBuffers, m_vertexBufferViews);
    cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
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
    m_currentDepthStencilBuffer.m_dsv.ptr = m_dsvStart.ptr + m_currentBackBufferId * m_dsvAllocator.GetCPUStride();

    WaitForDX12FenceValue(m_frameFenceValues[m_currentBackBufferId]);

    auto cmdList = GetCmdList();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Transition.pResource = m_currentRenderTarget.m_dx12Resource.resource.Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    cmdList->ResourceBarrier(1, &barrier);
}

void DX12Graphics::EndFrame(bool vsync)
{
    auto cmdList = GetCmdList();
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_renderTargets[m_currentBackBufferId].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);

    ExecuteCurrentCmdList();

    ThrowIfFailed(m_swapChain->Present(vsync ? 1 : 0, 0));

    m_frameFenceValues[m_currentBackBufferId] = GetCurrentDX12FenceValue() - 1;
    /*ThrowIfFailed(
        m_commandQueue->Signal(
            m_fence.Get(),
            m_currentFenceValue++
        )
    );*/

    m_currentBackBufferId = (m_currentBackBufferId + 1) % NUM_GRAPHICS_BACK_BUFFERS;
}

NAMESPACE_DX12_END