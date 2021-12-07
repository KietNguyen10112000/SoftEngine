#pragma once
#include <d3d11.h>

#include <IRenderer.h>

#include "PrimitiveTopology.h"

class ICamera;
class LightSystem;

class DX11Renderer : public IRenderer
{
private:
    friend class ShaderVar;
    friend class DynamicShaderVar;
    friend class RenderPipeline;

    friend class VertexShader;
    friend class PixelShader;
    friend class HullShader;
    friend class GeometryShader;

    friend class VertexBuffer;
    friend class IndexBuffer;

    friend class Texture1D;
    friend class Texture2D;
    friend class Texture3D;
    friend class TextureCube;

    friend class RenderTexture2D;
    friend class RenderTexture3D;
    friend class RenderTextureCube;

    friend class LightSystem;
    friend class DX11LightSystem;
    friend class DX11PostProcessor;
    friend class ShadowMap;

protected:
    ID3D11Device* m_d3dDevice = nullptr;
    ID3D11DeviceContext* m_d3dDeviceContext = nullptr;

    IDXGISwapChain* m_dxgiSwapChain = nullptr;
    IDXGIDevice* m_dxgiDevice = nullptr;
    IDXGIAdapter* m_dxgiAdapter = nullptr;
    IDXGIFactory* m_dxgiFactory = nullptr;


    ID3D11RasterizerState* m_rasterizerState = nullptr;
    ID3D11BlendState* m_blendState = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;

    D3D_FEATURE_LEVEL m_featureLevel = {};
    D3D11_TEXTURE2D_DESC m_bbDesc = {};
    D3D11_VIEWPORT m_viewport = {};

    ID3D11RenderTargetView* m_mainRtv = nullptr;
    ID3D11ShaderResourceView* m_mainRtvShader = nullptr;
    ID3D11DepthStencilView* m_dsv = nullptr;

    class DX11LightSystem* m_lightSystem = nullptr;
    class DX11PostProcessor* m_postProcessor = nullptr;

    ICamera* m_targetCam = nullptr;

public:
    DX11Renderer();
    virtual ~DX11Renderer();

};

class DeferredRenderer : public DX11Renderer
{
public:
    enum RTV_INDEX
    {
        COLOR                       = 0,
        POSITION_SPECULAR           = 1,
        NORMAL_SHININESS            = 2,
        METALLIC_ROUGHNESS_AO       = 3,
        LIGHTED_SCENE               = 4,
    };
    //for geometry pass
    /*
    [0]:    color
    [1]:    position + specular (float4 : float3 position + float specular)
    [2]:    normal + shininess (float4 : float3 normal + float shininess)
    [4]:    metallic + roughness + AO + not use yet
    */
    ID3D11RenderTargetView* m_rtv[18] = {};

    //for render pass
    ID3D11ShaderResourceView* m_rtvShader[18] = {};
    ID3D11ShaderResourceView* m_dsvShader = nullptr;

    RenderPipeline* m_currentRpl = nullptr;
    RenderPipeline* m_defaultRpl = nullptr;

    RenderPipeline* m_presentLightRpl = nullptr;

    int m_totalRtvUsed = 0;

    ShaderVar* m_viewPointSV = nullptr;
    VertexBuffer* m_screenQuad = nullptr;
    RenderPipeline* m_lastPresentRpl = nullptr;

    //not own
    ID3D11ShaderResourceView* m_lastSceneSrv = 0;
    //not own
    ID3D11RenderTargetView* m_lastSceneRtv = 0;

    IRenderableObject* m_skybox = nullptr;

#ifdef _DEBUG
    RenderPipeline* m_visualizeRpl = nullptr;

    RenderPipeline* m_shadowVisualizeRpl = nullptr;

    RenderPipeline* m_visualizeBPRRpl = nullptr;
#endif // _DEBUG

    bool m_doneLighting = false;

    using _FunCallType = void (*)(
        IRenderer*,
        RenderPipeline*,
        RenderPipeline*,
        VertexBuffer**, IndexBuffer**, uint32_t*);

    std::vector<std::pair<RenderPipeline*, _FunCallType>> m_rplStack;

public:
    //args[0] = hwnd
    DeferredRenderer(int numBuffer, int width, int height, int numArg, void** args);

    ~DeferredRenderer();

private:
    void CreateRtvs();

    void CreatePostProcessor();

    void VisualizePositionNormalDiffuseSpecular();
    void VisualizeShadowDepthMap();
    void VisualizePBR();

    void DoLighting();

public:
    virtual void CopyGlobal(IRenderer* renderer) override;
    virtual std::wstring ShaderDirectory() override;

public:
    void Present() override;

    void SetTargetCamera(ICamera* camera) override;

    inline ICamera* GetTargetCamera() override { return m_targetCam; };

    void ClearFrame(float color[4]) override;

    // Inherited via DX11Renderer
    virtual void SetRenderPipeline(RenderPipeline* rpl) override;

    virtual RenderPipeline*& DefaultRenderPipeline() override;

    virtual RenderPipeline*& CurrentRenderPipeline() override;

    virtual void Render(Object* obj) override;

    virtual void Render(VertexBuffer* vb) override;
    virtual void Render(VertexBuffer* vb, IndexBuffer* ib) override;
    virtual void Render(RenderPipeline* rpl, VertexBuffer* vb) override;
    virtual void Render(RenderPipeline* rpl, VertexBuffer* vb, IndexBuffer* ib) override;

    virtual void RenderInstance(RenderPipeline* rpl, 
        VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib) override;
    virtual void RenderInstance(RenderPipeline* rpl, 
        VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount) override;

public:
    virtual void RenderShadow(VertexBuffer* vb, IndexBuffer* ib) override;
    virtual void RenderInstanceShadow(VertexBuffer* vb, 
        VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib) override;

public:
    virtual void RedirectRenderPipeline(RenderPipeline* target,
        void (*func)(
            IRenderer*,
            RenderPipeline*,
            RenderPipeline*,
            VertexBuffer**, IndexBuffer**, uint32_t*)) override;
    virtual void RestoreRenderPipeline() override;

    virtual void NativeSetShaders(VertexShader*, HullShader*, DomainShader*, GeometryShader*, PixelShader*) override;

public:
    virtual void BeginTransparency() override;
    virtual void EndTransparency() override;

public:
    virtual void VisualizeBackgroundRenderPipeline(int arg) override;

    inline virtual class LightSystem* LightSystem() override { return (class LightSystem*)(m_lightSystem); };

    inline virtual class PostProcessor* PostProcessor() override { return (class PostProcessor*)(m_postProcessor); };

    // Inherited via DX11Renderer
    inline virtual void SetPrimitiveTopology(PRIMITIVE_TOPOLOGY topology) override 
    {
        m_d3dDeviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);
    };


    inline virtual void AttachSkyBox(IRenderableObject* skybox) override { m_skybox = skybox; };

public:
    inline virtual float GetRenderWidth() override { return m_bbDesc.Width; };
    inline virtual float GetRenderHeight() override { return m_bbDesc.Height; };

};