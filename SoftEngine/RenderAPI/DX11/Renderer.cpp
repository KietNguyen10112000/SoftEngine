#include "Renderer.h"
#include <Windows.h>

#pragma comment(lib,"d3d11.lib")

#include "DX11Global.h"
#include <File.h>

#include <RenderPipeline.h>
#include <Resource.h>
#include <Buffer.h>

#include <Components/Quad.h>

#include <IObject.h>

#include "LightSystem.h"

#include "DX11PostProcessor.h"

#ifdef SCREEN_SHADOW_BLUR
struct ___GaussianVariable
{
    uint32_t type;
    uint32_t halfWeightCount;

    float w;
    float h;

    float weights[16];
};

___GaussianVariable ___gaussianVar;
#endif

#define ThrowIfFailed(hr, msg) if (FAILED(hr)) throw msg L"\nThrow from File \"" __FILEW__ L"\", Line " _STRINGIZE(__LINE__) L"."

DX11Renderer::DX11Renderer()
{
    if (!DX11Global::renderer) DX11Global::renderer = this;
}

DX11Renderer::~DX11Renderer()
{
    if (m_postProcessor) delete m_postProcessor;

    m_rasterizerState->Release();
    m_dsv->Release();

    m_screenRtv->Release();
    m_mainRtv->Release();

    if (m_mainBuffer) m_mainBuffer->Release();
    if (m_screenBuffer) m_screenBuffer->Release();


    m_blendState->Release();
    m_samplerState->Release();
    m_d3dDeviceContext->Release();
    m_d3dDevice->Release();
    m_dxgiSwapChain->Release();
    m_dxgiFactory->Release();
    m_dxgiAdapter->Release();
    m_dxgiDevice->Release();

    if (m_mainRtvShader) m_mainRtvShader->Release();
}

DeferredRenderer::DeferredRenderer(int numBuffer, int width, int height, int numArg, void** args)
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL levels[] = {
        //D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0
    };

    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, levels,
        ARRAYSIZE(levels), D3D11_SDK_VERSION, &m_d3dDevice, &m_featureLevel, &m_d3dDeviceContext);

    //#if defined(DEBUG) || defined(_DEBUG)
    //    m_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_debug);
    //#endif

    m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_dxgiDevice);
    m_dxgiDevice->GetParent(__uuidof(IDXGIAdapter1), (void**)&m_dxgiAdapter);
    m_dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_dxgiFactory);

    UINT MSAACount = 1;
    UINT MSAAQuality = 0;

    if (numArg > 1) MSAACount = *(UINT*)args[1];

    auto format = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = m_d3dDevice->CheckMultisampleQualityLevels(format, MSAACount, &MSAAQuality);

    ThrowIfFailed(hr, L"CheckMultisampleQualityLevels() failed. Prefer setting down MSAA level.");


    //IDXGIFactory2* dxgiFactory2 = nullptr;
    //hr = m_dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    //m_d3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&m_d3dDevice1));
    //m_d3dDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&m_d3dDeviceContext1));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = numBuffer;
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;    //desc.BufferDesc.Width = width;
    //desc.BufferDesc.Height = height;
    //desc.BufferDesc.Format = format; //hdr will be DXGI_FORMAT_R10G10B10A2_UNORM and depth wiil be DXGI_FORMAT_D32_FLOAT
    //desc.BufferDesc.RefreshRate.Numerator = 120;
    //desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //desc.OutputWindow = (HWND)args[0];
    desc.SampleDesc.Count = MSAACount;
    desc.SampleDesc.Quality = MSAAQuality - 1;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    //desc.Windowed = TRUE;
    //desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    //if(MSAACount != 1)
        //desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; 
    //else
        //desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    //DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    //fsSwapChainDesc.Windowed = FALSE;
    //fsSwapChainDesc.RefreshRate.Numerator = 120;
    //fsSwapChainDesc.RefreshRate.Denominator = 1;

    hr = m_dxgiFactory->CreateSwapChainForHwnd(
        m_d3dDevice, (HWND)args[0], &desc, 0, 0, &m_dxgiSwapChain
    );

    ThrowIfFailed(hr, L"CreateSwapChain() failed.");

    //m_dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&m_dxgiSwapChain));
    //dxgiFactory2->Release();


    ID3D11Texture2D* buffer = NULL;

    m_dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
    buffer->GetDesc(&m_bbDesc);

    D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
    renderDesc.Format = format;
    renderDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderDesc.Texture2D.MipSlice = 0;

    //hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_mainRtv);

    hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_screenRtv);


    //====================create main rtv===========================================
    ID3D11Texture2D* mainbuffer = NULL;
    D3D11_TEXTURE2D_DESC mainbufferDesc = {};

    mainbufferDesc.Width = m_bbDesc.Width;
    mainbufferDesc.Height = m_bbDesc.Height;
    mainbufferDesc.MipLevels = 1;
    mainbufferDesc.ArraySize = 1;
    mainbufferDesc.Format = m_bbDesc.Format;
    mainbufferDesc.SampleDesc.Count = MSAACount;
    mainbufferDesc.SampleDesc.Quality = MSAAQuality - 1;
    mainbufferDesc.Usage = D3D11_USAGE_DEFAULT;
    mainbufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    mainbufferDesc.CPUAccessFlags = 0;
    mainbufferDesc.MiscFlags = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC mainsrvDesc = {};
    mainsrvDesc.Format = format;
    mainsrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    mainsrvDesc.Texture2D.MostDetailedMip = 0;
    mainsrvDesc.Texture2D.MipLevels = 1;

    m_d3dDevice->CreateTexture2D(&mainbufferDesc, 0, &mainbuffer);
    m_d3dDevice->CreateRenderTargetView(mainbuffer, &renderDesc, &m_mainRtv);
    m_d3dDevice->CreateShaderResourceView(mainbuffer, &mainsrvDesc, &m_mainRtvShader);

    m_mainBuffer = mainbuffer;
    //mainbuffer->Release();
    //====================create main rtv===========================================


    /*D3D11_SHADER_RESOURCE_VIEW_DESC rtvsrvDesc = {};
    rtvsrvDesc.Format = format;
    rtvsrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    rtvsrvDesc.Texture2D.MostDetailedMip = 0;
    rtvsrvDesc.Texture2D.MipLevels = 1;
    hr = m_d3dDevice->CreateShaderResourceView(buffer, &rtvsrvDesc, &m_mainRtvShader);*/

    m_screenBuffer = buffer;
    //buffer->Release();

    ThrowIfFailed(hr, L"CreateRenderTargetView() failed.");

    ID3D11Texture2D* pDepthStencil = NULL;
    D3D11_TEXTURE2D_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

    depthStencilDesc.Width = m_bbDesc.Width;
    depthStencilDesc.Height = m_bbDesc.Height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthStencilDesc.SampleDesc.Count = MSAACount;
    depthStencilDesc.SampleDesc.Quality = MSAAQuality - 1;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &pDepthStencil);

    ThrowIfFailed(hr, L"CreateTexture2D() failed.");

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    descDSV.Flags = 0;

    // Create the depth stencil view
    hr = m_d3dDevice->CreateDepthStencilView(pDepthStencil,
        &descDSV,
        &m_dsv);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = MSAACount == 1 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = m_d3dDevice->CreateShaderResourceView(pDepthStencil, &srvDesc, &m_dsvShader);

    ThrowIfFailed(hr, L"Failed.");

    pDepthStencil->Release();

    ThrowIfFailed(hr, L"CreateDepthStencilView() failed.");

    D3D11_RASTERIZER_DESC RasterizerDesc;
    ZeroMemory(&RasterizerDesc, sizeof(RasterizerDesc));
    RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    RasterizerDesc.CullMode = D3D11_CULL_BACK;//D3D11_CULL_NONE;
    RasterizerDesc.FrontCounterClockwise = FALSE;
    RasterizerDesc.DepthBias = 0;
    RasterizerDesc.SlopeScaledDepthBias = 0.0f;
    RasterizerDesc.DepthBiasClamp = 0.0f;
    RasterizerDesc.DepthClipEnable = TRUE;
    RasterizerDesc.ScissorEnable = FALSE;
    RasterizerDesc.MultisampleEnable = FALSE;
    RasterizerDesc.AntialiasedLineEnable = FALSE;

    //CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
    //    D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
    //    D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE,
    //    TRUE /*this is MSAA enable*/, FALSE);

    m_d3dDevice->CreateRasterizerState(&RasterizerDesc, &m_rasterizerState);
    m_d3dDeviceContext->RSSetState(m_rasterizerState);

    //enable alpha channel
    D3D11_BLEND_DESC BlendState;
    ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
    BlendState.AlphaToCoverageEnable = FALSE;
    BlendState.IndependentBlendEnable = TRUE;

    BlendState.RenderTarget[0].BlendEnable = TRUE;
    BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;//D3D11_BLEND_ONE;
    BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;//D3D11_BLEND_ZERO;
    BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ::memcpy(&BlendState.RenderTarget[1], &BlendState.RenderTarget[0], sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
    ::memcpy(&BlendState.RenderTarget[2], &BlendState.RenderTarget[0], sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
    ::memcpy(&BlendState.RenderTarget[3], &BlendState.RenderTarget[0], sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
    ::memcpy(&BlendState.RenderTarget[4], &BlendState.RenderTarget[0], sizeof(D3D11_RENDER_TARGET_BLEND_DESC));

    //BlendState.RenderTarget[0].BlendEnable = FALSE;
    BlendState.RenderTarget[1].BlendEnable = FALSE;
    BlendState.RenderTarget[2].BlendEnable = FALSE;
    BlendState.RenderTarget[3].BlendEnable = FALSE;
    BlendState.RenderTarget[4].BlendEnable = FALSE;
   

    m_d3dDevice->CreateBlendState(&BlendState, &m_blendState);

    float blendFactor[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
    UINT sampleMask = 0xffffffff;

    m_d3dDeviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);

    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.Height = height;
    m_viewport.Width = width;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_d3dDeviceContext->RSSetViewports(1, &m_viewport);

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerState);

    m_d3dDeviceContext->PSSetSamplers(0, 1, &m_samplerState);

    CreateRtvs();

    float cls[] = { 0,0,1,1 };

    m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, m_dsv);

    m_d3dDeviceContext->ClearRenderTargetView(m_mainRtv, cls);

    m_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

DeferredRenderer::~DeferredRenderer()
{
#ifdef SCREEN_SHADOW_BLUR
    Resource::Release(&m_shadowCombinePS);
    delete m_gaussianBlurVar;
    Resource::Release(&m_gaussianBlurPS);
    m_screenShadowBlurBufSRV1->Release();
    m_screenShadowBlurBufRTV1->Release();
    m_screenShadowBlurBufSRV2->Release();
    m_screenShadowBlurBufRTV2->Release();
#endif

    m_rtv[RTV_INDEX::COUNT] = 0;

    for (size_t i = 0; i < 8; i++)
    {
        if (m_rtv[i]) m_rtv[i]->Release();
        if (m_rtvShader[i]) m_rtvShader[i]->Release();
    }

    if(m_dsvShader) m_dsvShader->Release();

    RenderPipelineManager::Release(&m_presentLightRpl);

    if (m_lightSystem) delete m_lightSystem;

    delete m_viewPointSV;

    if (m_screenQuad) delete m_screenQuad;

    RenderPipelineManager::Release(&m_lastPresentRpl);

#ifdef _DEBUG
    RenderPipelineManager::Release(&m_visualizeRpl);
    RenderPipelineManager::Release(&m_shadowVisualizeRpl);
    RenderPipelineManager::Release(&m_visualizeBPRRpl);
#endif // _DEBUG
}

void DeferredRenderer::CreateRtvs()
{
    ID3D11Texture2D* buffer = NULL;

    D3D11_TEXTURE2D_DESC bufferDesc = {};

    auto format = DXGI_FORMAT_R32G32B32_FLOAT;

    bufferDesc.Width = m_bbDesc.Width;
    bufferDesc.Height = m_bbDesc.Height;
    bufferDesc.MipLevels = 1;
    bufferDesc.ArraySize = 1;
    bufferDesc.Format = format;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    D3D11_RENDER_TARGET_VIEW_DESC renderDesc = {};
    renderDesc.Format = format;
    renderDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderDesc.Texture2D.MipSlice = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    HRESULT hr = NULL;

    DXGI_FORMAT formats[] = {
        DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,


        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM
    };

    //m_totalRtvUsed = RTV_INDEX::COUNT;
    for (size_t i = 0; i < RTV_INDEX::COUNT; i++)
    {
        bufferDesc.Format = formats[i];
        renderDesc.Format = formats[i];
        srvDesc.Format = formats[i];

        hr = m_d3dDevice->CreateTexture2D(&bufferDesc, NULL, &buffer);

        hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_rtv[i]);

        hr = m_d3dDevice->CreateShaderResourceView(buffer, &srvDesc, &m_rtvShader[i]);

        buffer->Release();

        ThrowIfFailed(hr, L"Failed.");
    }

    Resource::ShaderDirectory() = ShaderDirectory();

    m_presentLightRpl = RenderPipelineManager::Get(
        R"(struct VS_INPUT
        {
            vec3 pos; POSITION, PER_VERTEX #
            vec2 textCoord; TEXTCOORD, PER_VERTEX #
        };)",
        L"VSQuad",
        //L"PSLight"
        //L"Shadow/v2/PSLight"
        L"PBRLighting/PSLight"
    );

    m_lightSystem = new class DX11LightSystem();

    PixelShaderCBuffer temp;
    m_viewPointSV = new ShaderVar(&temp, sizeof(PixelShaderCBuffer));

    m_screenQuad = NewScreenRectangle();

    /*bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = m_d3dDevice->CreateTexture2D(&bufferDesc, NULL, &buffer);

    hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_rtv[m_totalRtvUsed]);

    hr = m_d3dDevice->CreateShaderResourceView(buffer, &srvDesc, &m_rtvShader[m_totalRtvUsed]);

    buffer->Release();*/

    ThrowIfFailed(hr, L"Failed.");

    //rtv[3] as visualize for last result
    //m_totalRtvUsed++;

    m_lastPresentRpl = RenderPipelineManager::Get(
        R"(struct VS_INPUT
        {
            vec3 pos; POSITION, PER_VERTEX #
            vec2 textCoord; TEXTCOORD, PER_VERTEX #
        };)",
        L"VSQuad",
        L"PSPresent"
    );

#ifdef _DEBUG
    m_visualizeRpl = RenderPipelineManager::Get(
        R"(struct VS_INPUT
        {
            vec3 pos; POSITION, PER_VERTEX #
            vec2 textCoord; TEXTCOORD, PER_VERTEX #
        };)", 
        L"VSQuad", 
        L"PSVisualize"
    );

    m_shadowVisualizeRpl = RenderPipelineManager::Get(
        R"(struct VS_INPUT
        {
            vec3 pos; POSITION, PER_VERTEX #
            vec2 textCoord; TEXTCOORD, PER_VERTEX #
        };)",
        L"VSQuad",
        L"Shadow/PSVisualize"
    );

#endif // _DEBUG
    

    m_lastSceneSrv = m_rtvShader[LIGHTED_SCENE];
    m_lastSceneRtv = m_rtv[LIGHTED_SCENE];

#ifdef SCREEN_SHADOW_BLUR
    m_shadowCombinePS = Resource::Get<PixelShader>(L"Shadow/v2/PSCombineShadowColor");
    m_gaussianBlurPS = Resource::Get<PixelShader>(L"Shadow/v2/PSVariableGaussianBlur");

    ___gaussianVar.w = m_bbDesc.Width / 2;
    ___gaussianVar.h = m_bbDesc.Height / 2;
    ___gaussianVar.halfWeightCount = 5;
    const float weight[5] = {
        0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
    };
    ::memcpy(___gaussianVar.weights, weight, sizeof(float) * 5);
    m_gaussianBlurVar = new ShaderVar(&___gaussianVar, sizeof(___gaussianVar));

    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.Width = ___gaussianVar.w;
    bufferDesc.Height = ___gaussianVar.h;
    renderDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = m_d3dDevice->CreateTexture2D(&bufferDesc, NULL, &buffer);
    hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_screenShadowBlurBufRTV1);
    hr = m_d3dDevice->CreateShaderResourceView(buffer, &srvDesc, &m_screenShadowBlurBufSRV1);
    buffer->Release();

    hr = m_d3dDevice->CreateTexture2D(&bufferDesc, NULL, &buffer);
    hr = m_d3dDevice->CreateRenderTargetView(buffer, &renderDesc, &m_screenShadowBlurBufRTV2);
    hr = m_d3dDevice->CreateShaderResourceView(buffer, &srvDesc, &m_screenShadowBlurBufSRV2);
    buffer->Release();
#endif

    CreatePostProcessor();
}

void DeferredRenderer::CopyGlobal(IRenderer* renderer)
{
    DX11Global::renderer = dynamic_cast<DX11Renderer*>(renderer);
}

std::wstring DeferredRenderer::ShaderDirectory()
{
#if defined(_DEBUG) || defined(LOCAL_RELEASE)
    std::wstring cur = __FILEW__;
    StandardPath(cur);
    return CombinePath(cur, L"../HLSL/DeferredShading");
#else
    wchar_t buffer[_MAX_DIR] = {};
    GetCurrentDirectoryW(_MAX_DIR, buffer);
    std::wstring currentDir = buffer;
    return CombinePath(currentDir, L"DX11/HLSL");
#endif // _DEBUG
}

void DeferredRenderer::DoLighting()
{
    m_dataPassToPixelShader.viewPoint = m_targetCam->GetPosition();
    m_dataPassToPixelShader.mvp = m_targetCam->MVP();
    m_dataPassToPixelShader.invMVP = GetInverse(m_dataPassToPixelShader.mvp);
    //Vec4 temp = { pos.x, pos.y, pos.z, 1 };

    m_viewPointSV->Update(&m_dataPassToPixelShader, sizeof(m_dataPassToPixelShader));

    //m_lastSceneRtv == LIGHTED_SCENE
    //shadowColor == SHADOW_COLOR
    m_d3dDeviceContext->OMSetRenderTargets(2, &m_rtv[RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW], 0);

//#ifndef SCREEN_SHADOW_BLUR
//    if (m_skybox) m_skybox->Render(this);
//#endif

    m_d3dDeviceContext->PSSetConstantBuffers(0, 1, &m_viewPointSV->GetNativeHandle());
    m_d3dDeviceContext->PSSetConstantBuffers(1, 1, &m_lightSystem->m_lightSysInfo->GetNativeHandle());

    //m_d3dDeviceContext->GenerateMips(m_lightSystem->m_shadowDepthMapSrv);

    //t0, 1, 2
    m_d3dDeviceContext->PSSetShaderResources(0, 1, &m_lightSystem->m_lightsBufferSrv);
    m_d3dDeviceContext->PSSetShaderResources(1, 1, &m_lightSystem->m_shadowLightsBufferSrv);
    m_d3dDeviceContext->PSSetShaderResources(2, 1, &m_lightSystem->m_shadowDepthMapSrv);

    //....
    m_d3dDeviceContext->PSSetShaderResources(3, 1, &m_dsvShader);
    m_d3dDeviceContext->PSSetShaderResources(4, 4/*RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW*/, m_rtvShader);

#ifndef SCREEN_SHADOW_BLUR
    if (m_skybox) m_skybox->Render(this);
#endif

#ifdef SCREEN_SHADOW_BLUR
    //present to 1 shadow image, 1 light image
#endif
    Render(m_presentLightRpl, m_screenQuad);

#ifdef SCREEN_SHADOW_BLUR
    auto vs = m_presentLightRpl->GetVS();

    D3D11_VIEWPORT vp = {};
    vp.Width = ___gaussianVar.w;
    vp.Height = ___gaussianVar.h;
    //D3D11_VIEWPORT vps[] = { vp, vp };
    m_d3dDeviceContext->RSSetViewports(1, &vp);
    //==================================blur shadow image=================================================
    ___gaussianVar.type = 0;
    m_gaussianBlurVar->Update(&___gaussianVar, sizeof(___gaussianVar));
    m_d3dDeviceContext->PSSetConstantBuffers(2, 1, &m_gaussianBlurVar->GetNativeHandle());

    m_d3dDeviceContext->IASetInputLayout(vs->GetNativeLayoutHandle());
    m_d3dDeviceContext->VSSetShader(vs->GetNativeHandle(), 0, 0);
    m_d3dDeviceContext->PSSetShader(m_gaussianBlurPS->GetNativeHandle(), 0, 0);

    //first pass
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_screenShadowBlurBufRTV1, 0);
    m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_rtvShader[RTV_INDEX::SHADOW_COLOR]);
    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &m_screenQuad->GetNativeHandle(), 
        &m_screenQuad->Stride(), &m_screenQuad->Offset());
    m_d3dDeviceContext->Draw(m_screenQuad->Count(), 0);

    //unset
    m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_rtvShader[RTV_INDEX::COUNT]);

    //second pass
    ___gaussianVar.type = 1;
    m_gaussianBlurVar->Update(&___gaussianVar, sizeof(___gaussianVar));
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_screenShadowBlurBufRTV2, 0);
    m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_screenShadowBlurBufSRV1);
    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &m_screenQuad->GetNativeHandle(),
        &m_screenQuad->Stride(), &m_screenQuad->Offset());
    m_d3dDeviceContext->Draw(m_screenQuad->Count(), 0);

    m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_rtvShader[RTV_INDEX::COUNT]);


    m_d3dDeviceContext->RSSetViewports(1, &m_viewport);
    //combine
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_lastSceneRtv, 0);
    if (m_skybox) m_skybox->Render(this);

    m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_rtvShader[RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW]);
    m_d3dDeviceContext->PSSetShaderResources(5, 1, &m_screenShadowBlurBufSRV2);
    m_d3dDeviceContext->PSSetShader(m_shadowCombinePS->GetNativeHandle(), 0, 0);
    
    
    m_d3dDeviceContext->IASetInputLayout(vs->GetNativeLayoutHandle());
    m_d3dDeviceContext->VSSetShader(vs->GetNativeHandle(), 0, 0);
    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &m_screenQuad->GetNativeHandle(), &m_screenQuad->Stride(), &m_screenQuad->Offset());
    m_d3dDeviceContext->Draw(m_screenQuad->Count(), 0);
#endif

    m_doneLighting = true;
}

void DeferredRenderer::Present()
{
    //m_d3dDeviceContext->OMSetRenderTargets(8, &m_rtv[RTV_INDEX::COUNT], nullptr);
    if (!m_doneLighting) DoLighting();

    if (m_currentRpl)
    {
        m_lastPresentRpl->Use();
        m_currentRpl = 0;
    }
    
    if (m_postProcessor && m_postProcessor->Run())
    {
        m_d3dDeviceContext->OMSetRenderTargets(1, &m_screenRtv, 0);
        m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_mainRtvShader);
        Render(m_lastPresentRpl, m_screenQuad);
    }
    else
    {
        m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, 0);
        m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_lastSceneSrv);
        Render(m_lastPresentRpl, m_screenQuad);
        PresentLastFrame();
    }

    //m_dxgiSwapChain->Present(1, 0);
    m_d3dDeviceContext->OMSetRenderTargets(8, &m_rtv[RTV_INDEX::COUNT], nullptr);
    m_d3dDeviceContext->PSSetShaderResources(0, 15, &m_rtvShader[RTV_INDEX::COUNT]);
    

}
void DeferredRenderer::SetTargetCamera(ICamera* camera)
{
    m_targetCam = camera;
    m_d3dDeviceContext->VSSetConstantBuffers(CAMERA_MVP_SHADER_LOCATION, 1, &ICamera::shaderMVP->GetNativeHandle());
}

void DeferredRenderer::ClearFrame(float color[4])
{
    //static float cls[4] = { 0.8,0.3,0.5,1 };

    m_doneLighting = false;

    m_d3dDeviceContext->ClearRenderTargetView(m_mainRtv, color);
    m_d3dDeviceContext->ClearDepthStencilView(m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    //m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, m_dsv);

    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[0], color);
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[1], color);
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[2], color);
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[3], color);
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[4], color);

#ifdef SCREEN_SHADOW_BLUR
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[RTV_INDEX::SHADOW_COLOR], color);
    m_d3dDeviceContext->ClearRenderTargetView(m_rtv[RTV_INDEX::LIGHTED_SCENE_WITH_SHADOW], color);

    m_d3dDeviceContext->ClearRenderTargetView(m_screenShadowBlurBufRTV1, color);
    m_d3dDeviceContext->ClearRenderTargetView(m_screenShadowBlurBufRTV2, color);
#endif

    m_d3dDeviceContext->OMSetRenderTargets(RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW, m_rtv, m_dsv);

    if (m_targetCam)
    {
        ICamera::cameraBuf.mvp = m_targetCam->MVP();
        ICamera::cameraBuf.invMVP = GetInverse(ICamera::cameraBuf.mvp);
        ICamera::cameraBuf.proj = m_targetCam->ProjectionMatrix();
        ICamera::cameraBuf.view = m_targetCam->ViewMatrix();
        ICamera::shaderMVP->Update(&ICamera::cameraBuf, sizeof(ICamera::cameraBuf));
    }
}

void DeferredRenderer::SetRenderPipeline(RenderPipeline* rpl)
{
    m_currentRpl = rpl;
}

RenderPipeline*& DeferredRenderer::DefaultRenderPipeline()
{
    return m_defaultRpl;
}

RenderPipeline*& DeferredRenderer::CurrentRenderPipeline()
{
    return m_currentRpl;
}


void DeferredRenderer::Render(Object* obj)
{
}

void DeferredRenderer::Render(VertexBuffer* vb)
{
}

void DeferredRenderer::Render(VertexBuffer* vb, IndexBuffer* ib)
{
}

void DeferredRenderer::Render(RenderPipeline* rpl, VertexBuffer* vb)
{
    if (m_rplStack.size() != 0)
    {
        auto& back = m_rplStack.back();
        back.second(this, back.first, rpl, &vb, 0, 0);
    }
    else if (m_currentRpl != rpl)
    {
        m_currentRpl = rpl;
        rpl->Use();
    }

    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &vb->GetNativeHandle(), &vb->Stride(), &vb->Offset());
    m_d3dDeviceContext->Draw(vb->Count(), 0);
}

void DeferredRenderer::Render(RenderPipeline* rpl, VertexBuffer* vb, IndexBuffer* ib)
{
    if (m_rplStack.size() != 0)
    {
        auto& back = m_rplStack.back();
        back.second(this, back.first, rpl, &vb, &ib, 0);
    }
    else if (m_currentRpl != rpl)
    {
        m_currentRpl = rpl;
        rpl->Use();
    }
    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &vb->GetNativeHandle(), &vb->Stride(), &vb->Offset());
    m_d3dDeviceContext->IASetIndexBuffer(ib->GetNativeHandle(), ib->GetNativeFormat(), 0);
    m_d3dDeviceContext->DrawIndexed(ib->Count(), 0, 0);
}

void DeferredRenderer::RenderInstance(RenderPipeline* rpl, 
    VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib)
{
    if (m_rplStack.size() != 0)
    {
        auto& back = m_rplStack.back();
        VertexBuffer* arg[] = { vb,instanceBuffer };
        back.second(this, back.first, rpl, arg, &ib, &instanceCount);
    }
    else if (m_currentRpl != rpl)
    {
        m_currentRpl = rpl;
        rpl->Use();
    }

    ID3D11Buffer* vbs[] = { vb->GetNativeHandle(),instanceBuffer->GetNativeHandle() };
    uint32_t strides[] = { vb->Stride(), instanceBuffer->Stride() };
    uint32_t offsets[] = { vb->Offset(), instanceBuffer->Offset() };

    m_d3dDeviceContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);
    m_d3dDeviceContext->IASetIndexBuffer(ib->GetNativeHandle(), ib->GetNativeFormat(), 0);

    m_d3dDeviceContext->DrawIndexedInstanced(ib->Count(), instanceCount, 0, 0, 0);
}

void DeferredRenderer::RenderInstance(RenderPipeline* rpl, 
    VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount)
{
    if (m_rplStack.size() != 0)
    {
        auto& back = m_rplStack.back();
        VertexBuffer* arg[] = { vb,instanceBuffer };
        back.second(this, back.first, rpl, arg, 0, &instanceCount);
    }
    else if (m_currentRpl != rpl)
    {
        m_currentRpl = rpl;
        rpl->Use();
    }

    ID3D11Buffer* vbs[] = { vb->GetNativeHandle(),instanceBuffer->GetNativeHandle() };
    uint32_t strides[] = { vb->Stride(), instanceBuffer->Stride() };
    uint32_t offsets[] = { vb->Offset(), instanceBuffer->Offset() };

    m_d3dDeviceContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);

    m_d3dDeviceContext->DrawInstanced(vb->Count(), instanceCount, 0, 0);
}

void DeferredRenderer::RenderShadow(VertexBuffer* vb, IndexBuffer* ib)
{
    m_d3dDeviceContext->IASetVertexBuffers(0, 1, &vb->GetNativeHandle(), &vb->Stride(), &vb->Offset());
    m_d3dDeviceContext->IASetIndexBuffer(ib->GetNativeHandle(), ib->GetNativeFormat(), 0);
    m_d3dDeviceContext->DrawIndexed(ib->Count(), 0, 0);
}

void DeferredRenderer::RenderInstanceShadow(VertexBuffer* vb, 
    VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib)
{

}

void DeferredRenderer::RedirectRenderPipeline(RenderPipeline* target, 
    void(*func)(IRenderer*, RenderPipeline*, RenderPipeline*, VertexBuffer**, IndexBuffer**, uint32_t*))
{
    m_rplStack.push_back({ target, func });
}

void DeferredRenderer::RestoreRenderPipeline()
{
    m_rplStack.pop_back();
}

void DeferredRenderer::NativeSetShaders(VertexShader* vs, HullShader* hs, DomainShader* ds, GeometryShader* gs, PixelShader* ps)
{
    m_d3dDeviceContext->IASetInputLayout(vs->GetNativeLayoutHandle());

    m_d3dDeviceContext->VSSetShader(vs->GetNativeHandle(), 0, 0);

    if (hs && ds)
    {
        m_d3dDeviceContext->HSSetShader(hs->GetNativeHandle(), 0, 0);
        m_d3dDeviceContext->DSSetShader(ds->GetNativeHandle(), 0, 0);
    }
    
    if(gs) m_d3dDeviceContext->GSSetShader(gs->GetNativeHandle(), 0, 0);

    m_d3dDeviceContext->PSSetShader(ps->GetNativeHandle(), 0, 0);
}

void DeferredRenderer::BeginTransparency()
{
    if(!m_doneLighting) DoLighting();
    //m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, 0);
    //m_d3dDeviceContext->PSSetShaderResources(4, 1, &m_lastSceneSrv);
    //Render(m_lastPresentRpl, m_screenQuad);
    m_d3dDeviceContext->PSSetShaderResources(3, 1, &m_rtvShader[RTV_INDEX::COUNT]);
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_lastSceneRtv, m_dsv);
    //m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, m_dsv);
}

void DeferredRenderer::EndTransparency()
{
    m_d3dDeviceContext->PSSetShaderResources(0, RTV_INDEX::COUNT + 4, &m_rtvShader[RTV_INDEX::COUNT]);
    m_d3dDeviceContext->OMSetRenderTargets(RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW, &m_rtv[RTV_INDEX::COUNT], nullptr);
}


void DeferredRenderer::BeginUI(int arg)
{
    m_d3dDeviceContext->OMSetRenderTargets(1, arg > 1 ? &m_screenRtv : &m_mainRtv, arg > 0 ? m_dsv : 0);
}

void DeferredRenderer::EndUI()
{
}

void DeferredRenderer::PresentLastFrame()
{
    m_d3dDeviceContext->CopyResource(m_screenBuffer, m_mainBuffer);
}

void DeferredRenderer::VisualizePositionNormalDiffuseSpecular()
{
#ifdef _DEBUG
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, 0);

    m_d3dDeviceContext->PSSetShaderResources(3, 1, &m_dsvShader);
    m_d3dDeviceContext->PSSetShaderResources(4, RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW, m_rtvShader);
    
    Render(m_visualizeRpl, m_screenQuad);

    m_d3dDeviceContext->PSSetShaderResources(3, RTV_INDEX::COUNT + 1, &m_rtvShader[RTV_INDEX::COUNT]);

    //m_dxgiSwapChain->Present(0, 0);

    m_d3dDeviceContext->OMSetRenderTargets(RTV_INDEX::COUNT, &m_rtv[RTV_INDEX::COUNT], nullptr);
#endif
}

void DeferredRenderer::VisualizeShadowDepthMap()
{
#ifdef _DEBUG
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, 0);

    //t0, 1, 2
    m_d3dDeviceContext->PSSetShaderResources(0, 1, &m_lightSystem->m_lightsBufferSrv);
    m_d3dDeviceContext->PSSetShaderResources(1, 1, &m_lightSystem->m_shadowLightsBufferSrv);
    m_d3dDeviceContext->PSSetShaderResources(2, 1, &m_lightSystem->m_shadowDepthMapSrv);

    //....
    m_d3dDeviceContext->PSSetShaderResources(3, 1, &m_dsvShader);
    m_d3dDeviceContext->PSSetShaderResources(4, RTV_INDEX::LIGHTED_SCENE_WITHOUT_SHADOW - 1, m_rtvShader);

    Render(m_shadowVisualizeRpl, m_screenQuad);

    //m_d3dDeviceContext->PSSetShaderResources(0, RTV_INDEX::COUNT + 2, &m_rtvShader[RTV_INDEX::COUNT]);

    //m_dxgiSwapChain->Present(0, 0);

    m_d3dDeviceContext->PSSetShaderResources(0, 8, &m_rtvShader[RTV_INDEX::COUNT]);
    m_d3dDeviceContext->OMSetRenderTargets(3, &m_rtv[RTV_INDEX::COUNT], nullptr);
#endif
}

void DeferredRenderer::VisualizePBR()
{
#ifdef _DEBUG
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRtv, 0);

    m_d3dDeviceContext->PSSetShaderResources(3, 1, &m_dsvShader);
    m_d3dDeviceContext->PSSetShaderResources(4, RTV_INDEX::COUNT, m_rtvShader);

    Render(m_visualizeBPRRpl, m_screenQuad);

    m_d3dDeviceContext->PSSetShaderResources(3, 8 - 3, &m_rtvShader[RTV_INDEX::COUNT]);

    //m_dxgiSwapChain->Present(0, 0);

    m_d3dDeviceContext->OMSetRenderTargets(RTV_INDEX::COUNT, &m_rtv[RTV_INDEX::COUNT], nullptr);
#endif
}

void DeferredRenderer::VisualizeBackgroundRenderPipeline(int arg)
{
#ifdef _DEBUG
    switch (arg)
    {
    case 0:
        VisualizePositionNormalDiffuseSpecular();
        break;
    case 1:
        VisualizeShadowDepthMap();
        break;
    case 2:
        VisualizePBR();
        break;
    default:
        break;
    }
#endif
}

void DeferredRenderer::CreatePostProcessor()
{
    ////dont need these rtv for post processing
    m_postProcessor = new DX11PostProcessor(this);
}