#include "DX12DebugGraphics.h"

#include "FileSystem/FileUtils.h"

#include "SubSystems/Rendering/RenderingSystem.h"
#include "Objects/Scene/Scene.h"

#define CPP
#include "Shaders/TypeDef.hlsli"

NAMESPACE_DX12_BEGIN

struct DX12DebugGraphics_ObjectCBuffer
{
    ObjectData Object;
    Vec4     Color;
};

DX12DebugGraphics::DX12DebugGraphics(DX12Graphics* graphics) : m_graphics(graphics)
{
	m_device = graphics->m_device.Get();
    InitCubeRenderer();
}

DX12DebugGraphics::~DX12DebugGraphics()
{
    m_cubeParams.Destroy();
}

void DX12DebugGraphics::InitCubeRenderer()
{
    {
        byte* vsBytes, * psBytes;
        size_t vsSize, psSize;

        FileUtils::ReadFile("Shaders/DebugGraphics/Cube.vs.cso", vsBytes, vsSize);
        FileUtils::ReadFile("Shaders/DebugGraphics/Cube.ps.cso", psBytes, psSize);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        psoDesc.pRootSignature = m_graphics->m_renderRooms.m_rootSignature.Get();

        psoDesc.VS.pShaderBytecode = vsBytes;
        psoDesc.VS.BytecodeLength = vsSize;

        psoDesc.PS.pShaderBytecode = psBytes;
        psoDesc.PS.BytecodeLength = psSize;

        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0; // must be the same sample description as the swapchain and depth/stencil buffer
        psoDesc.SampleMask = 0xffffffff;

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;

        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;//D3D11_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;//D3D11_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.NumRenderTargets = 1;

        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState.DepthEnable = TRUE; // enable depth testing
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // can write depth data to all of the depth/stencil buffer
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // pixel fragment passes depth test if destination pixel's depth is less than pixel fragment's
        psoDesc.DepthStencilState.StencilEnable = FALSE; // disable stencil test
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; // a default stencil read mask (doesn't matter at this point since stencil testing is turned off)
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // a default stencil write mask (also doesn't matter)
        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, again does not really matter since stencil testing is turned off
        { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        psoDesc.DepthStencilState.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
        psoDesc.DepthStencilState.BackFace = defaultStencilOp;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_cubePSO)));

        FileUtils::FreeBuffer(vsBytes);
        FileUtils::FreeBuffer(psBytes);
    }

    {
        size_t constBufSizes[] = {
            MemoryUtils::Align<256>(sizeof(ObjectData)),
        };

        m_cubeParams.Init(m_device, 32, 32, 2, 1, constBufSizes, 0, 0, 0, &m_graphics->m_synchObject);
    }

}

void DX12DebugGraphics::DrawAABox(const AABox& aaBox, const Vec4& color)
{
    auto center = aaBox.GetCenter();
    auto dims = aaBox.GetDimensions();

    auto pos = center - dims / 2.0f;

    Mat4 transform = Mat4::Identity();
    transform *= Mat4::Scaling(dims);
    transform *= Mat4::Translation(pos);

    {
        // write all data for rendering
        auto head = m_cubeParams.AllocateConstantBufferWriteHead();
        auto objectData = (DX12DebugGraphics_ObjectCBuffer*)m_cubeParams.GetConstantBuffer(head, 2);
        objectData->Object.transform = transform;
        objectData->Color = color;
    }

    if (m_cubeParams.IsNeedFlush())
    {
        RenderCubes();
    }
}

void DX12DebugGraphics::DrawCube(const Box& box, const Vec4& color)
{
    auto dt = m_graphics->GetRenderingSystem()->GetScene()->Dt();
    {
        static auto transform = Mat4::Identity();

        // write all data for rendering
        auto head = m_cubeParams.AllocateConstantBufferWriteHead();
        auto objectData = (ObjectData*)m_cubeParams.GetConstantBuffer(head, 2);

        transform *= 
              Mat4::Rotation(Vec3::LEFT,    dt * PI / 5.0f)
            * Mat4::Rotation(Vec3::FORWARD, dt * PI / 3.0f)
            * Mat4::Rotation(Vec3::UP,      -dt * PI / 6.0f);

        objectData->transform = transform;
    }

    if (m_cubeParams.IsNeedFlush())
    {
        RenderCubes();
    }
}

void DX12DebugGraphics::DrawCube(const Mat4& transform, const Vec4& color)
{
    {
        // write all data for rendering
        auto head = m_cubeParams.AllocateConstantBufferWriteHead();
        auto objectData = (DX12DebugGraphics_ObjectCBuffer*)m_cubeParams.GetConstantBuffer(head, 2);
        objectData->Object.transform = transform;
        objectData->Color = color;
    }

    if (m_cubeParams.IsNeedFlush())
    {
        RenderCubes();
    }
}

void DX12DebugGraphics::DrawSphere(const Sphere& sphere, const Vec4& color)
{
}

void DX12DebugGraphics::BeginFrame()
{
}

void DX12DebugGraphics::EndFrame()
{
    
}

void DX12DebugGraphics::BeginCamera()
{
}

void DX12DebugGraphics::EndCamera()
{
    while (m_cubeParams.IsRemainBatch())
    {
        RenderCubes();
    }
}

void DX12DebugGraphics::RenderCubes()
{
    // render
    auto cmdList = m_graphics->m_commandList;
    cmdList->SetPipelineState(m_cubePSO.Get());
    cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //m_sceneParams.BeginRenderingAsBuiltInParam(cmdList, m_builtInCBVs, m_builtInSRVs);
    //m_cameraParams.BeginRenderingAsBuiltInParam(cmdList, m_builtInCBVs + 1, m_builtInSRVs + 1);

    auto& builtInCBVs = m_graphics->m_builtInCBVs;
    m_cubeParams.RenderBatch(
        m_graphics,
        m_device,
        m_graphics->m_commandQueue.Get(),
        cmdList,
        &m_graphics->m_renderRooms,
        2, builtInCBVs,
        0, 0,
        [&]()
        {
            cmdList->DrawInstanced(36, 1, 0, 0);
        }
    );

    //m_cameraParams.EndRenderingAsBuiltInParam(m_graphics->m_commandQueue.Get());
    //m_sceneParams.EndRenderingAsBuiltInParam(m_graphics->m_commandQueue.Get());
}

NAMESPACE_DX12_END