#include "RenderingSystem.h"

#include "Core/Thread/Thread.h"

#include "Components/Camera.h"

#include "Modules/Graphics/Graphics.h"

#include "Modules/Graphics/Detail/DX12/Shaders/Common/TypeDef.hlsli"

NAMESPACE_BEGIN

//struct TestConstantBuffer
//{
//    Mat4 vp;
//    Mat4 dummy;
//};

CameraData g_cameraData;
ObjectData g_objectData;

RenderingSystem::RenderingSystem(Scene* scene) : MainSystem(scene)
{
	auto graphics = Graphics::Get();

    struct Vertex
    {
        Vec3 position;
        Vec3 color;
    };

    Vertex cubeVertices[] =
    {
        { Vec3(-1.0f, -1.0f, -1.0f), Vec3(0, 1, 1) },
        { Vec3(-1.0f,  1.0f, -1.0f), Vec3(0, 0, 1) },
        { Vec3(1.0f,  1.0f, -1.0f), Vec3(1, 0, 1) },
        { Vec3(1.0f, -1.0f, -1.0f), Vec3(1, 1, 1) },

        { Vec3(-1.0f, -1.0f, 1.0f), Vec3(1, 1, 1) },
        { Vec3(1.0f, -1.0f, 1.0f), Vec3(0, 1, 1) },
        { Vec3(1.0f,  1.0f, 1.0f), Vec3(0, 0, 1) },
        { Vec3(-1.0f,  1.0f, 1.0f), Vec3(1, 0, 1) },

        { Vec3(-1.0f, 1.0f, -1.0f), Vec3(0, 1, 1) },
        { Vec3(-1.0f, 1.0f,  1.0f), Vec3(0, 0, 1) },
        { Vec3(1.0f, 1.0f,  1.0f), Vec3(1, 0, 1) },
        { Vec3(1.0f, 1.0f, -1.0f), Vec3(1, 1, 1) },

        { Vec3(-1.0f, -1.0f, -1.0f), Vec3(1, 1, 1) },
        { Vec3(1.0f, -1.0f, -1.0f), Vec3(0, 1, 1) },
        { Vec3(1.0f, -1.0f,  1.0f), Vec3(0, 0, 1) },
        { Vec3(-1.0f, -1.0f,  1.0f), Vec3(1, 0, 1) },

        { Vec3(-1.0f, -1.0f,  1.0f), Vec3(0, 1, 1) },
        { Vec3(-1.0f,  1.0f,  1.0f), Vec3(0, 0, 1) },
        { Vec3(-1.0f,  1.0f, -1.0f), Vec3(1, 0, 1) },
        { Vec3(-1.0f, -1.0f, -1.0f), Vec3(1, 1, 1) },

        { Vec3(1.0f, -1.0f, -1.0f), Vec3(0, 1, 1) },
        { Vec3(1.0f,  1.0f, -1.0f), Vec3(0, 0, 1) },
        { Vec3(1.0f,  1.0f,  1.0f), Vec3(1, 0, 1) },
        { Vec3(1.0f, -1.0f,  1.0f), Vec3(1, 1, 1) },
    };

    // Create index buffer:
    unsigned short cubeIndices[] =
    {
        0,  1,  2,
        0,  2,  3,

        // Back Face
        4,  5,  6,
        4,  6,  7,

        // Top Face
        8,  9, 10,
        8, 10, 11,

        // Bottom Face
        12, 13, 14,
        12, 14, 15,

        // Left Face
        16, 17, 18,
        16, 18, 19,

        // Right Face
        20, 21, 22,
        20, 22, 23
    };

    Vertex vList[36] = {};
    const int vBufferSize = sizeof(vList);

    for (size_t i = 0; i < 36; i++)
    {
        vList[i] = cubeVertices[cubeIndices[i]];
    }

	GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC vbDesc = {};
    vbDesc.count = 36;
    vbDesc.stride = sizeof(Vertex);
    m_testVertexBuffer = graphics->CreateVertexBuffer(vbDesc);
    m_testVertexBuffer->UpdateBuffer(vList, sizeof(vList));

    GRAPHICS_CONSTANT_BUFFER_DESC cbDesc1 = {};
    cbDesc1.bufferSize = sizeof(CameraData);
    cbDesc1.perferNumRoom = 3;
    graphics->CreateConstantBuffers(1, &cbDesc1, &m_testCameraConstantBuffer);

    GRAPHICS_CONSTANT_BUFFER_DESC cbDesc2 = {};
    cbDesc2.bufferSize = sizeof(ObjectData);
    cbDesc2.perferNumRoom = 4096;
    graphics->CreateConstantBuffers(1, &cbDesc2, &m_testObjectConstantBuffer);

    GRAPHICS_PIPELINE_DESC pipelineDesc = {};
    pipelineDesc.preferRenderCallPerFrame = 4096;
    pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.vs = "Test.vs";
    pipelineDesc.ps = "Test.ps";

    pipelineDesc.inputDesc.numElements = 2;
    pipelineDesc.inputDesc.elements[0] = { 
        "POSITION", 
        0, 
        GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT, 
        0, 
        0, 
        GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA, 
        0 
    };
    pipelineDesc.inputDesc.elements[1] = { 
        "COLOR", 
        0, 
        GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT, 
        0, 
        sizeof(Vec3), 
        GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        0 
    };

    pipelineDesc.outputDesc.numRenderTarget = 1;
    pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;

    m_testPipeline = graphics->CreateRasterizerPipeline(pipelineDesc);

    g_objectData.transform = Mat4::Identity();
}

RenderingSystem::~RenderingSystem()
{
}

void RenderingSystem::AddCamera(Camera* camera)
{
	assert(camera->m_activeID == INVALID_ID);

	camera->m_activeID = m_activeCamera.size();
	m_activeCamera.push_back(camera);
}

void RenderingSystem::RemoveCamera(Camera* camera)
{
	assert(camera->m_activeID != INVALID_ID);

	STD_VECTOR_ROLL_TO_FILL_BLANK(m_activeCamera, camera, m_activeID);
	camera->m_activeID = INVALID_ID;
}

void RenderingSystem::DisplayCamera(Camera* camera, const GRAPHICS_VIEWPORT& viewport)
{
	m_displayingCamera.push_back({ camera, viewport });
}

void RenderingSystem::HideCamera(Camera* camera)
{
	size_t i = 0;
Begin:
	i = 0;
	for (auto& dc : m_displayingCamera)
	{
		if (dc.camera == camera)
		{
			STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_displayingCamera, i);
			goto Begin;
		}
		i++;
	}
}

void RenderingSystem::AddComponent(MainComponent* comp)
{
	m_bvh.RecordAddComponent(comp);
}

void RenderingSystem::RemoveComponent(MainComponent* comp)
{
	m_bvh.RecordRemoveComponent(comp);
}

void RenderingSystem::OnObjectTransformChanged(MainComponent* comp)
{
	m_bvh.RecordRefreshComponent(comp);
}

void RenderingSystem::Iteration(float dt)
{
	m_bvh.Reconstruct(5'000'000);

	ProcessAllCmds(GetPrevServer(), this);

	auto graphics = Graphics::Get();
	graphics->BeginFrame();

	// do render

	auto screenRT = graphics->GetScreenRenderTarget();
	auto screenDS = graphics->GetScreenDepthStencilBuffer();

	//graphics->SetRenderTarget(1, &screenRT, screenDS);
	graphics->ClearRenderTarget(screenRT, { 0.1f, 0.5f, 0.5f, 1.0f }, 0, 0);
	graphics->ClearDepthStencil(screenDS, 0, 0);

    graphics->SetRenderTarget(1, &screenRT, screenDS);

    g_cameraData.vp = Mat4::Identity().SetLookAtLH({ 5,0,0 }, { 0,0,0 }, Vec3::UP) 
        * Mat4::Identity().SetPerspectiveFovLH(PI / 3.0f, 16 / 9.0f, 0.5f, 1000.0f);
    g_objectData.transform *= Mat4::Rotation(Vec3::UP, dt * PI / 3.0f) * Mat4::Rotation(Vec3::RIGHT, dt * PI / 4.0f);
    m_testCameraConstantBuffer->UpdateBuffer(&g_cameraData, sizeof(g_cameraData));
    m_testObjectConstantBuffer->UpdateBuffer(&g_objectData, sizeof(g_objectData));

    auto params = m_testPipeline->PrepareRenderParams();
    params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_testCameraConstantBuffer);
    params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_testObjectConstantBuffer);
    graphics->SetGraphicsPipeline(m_testPipeline.get());
    
    auto vb = m_testVertexBuffer.get();
    graphics->DrawInstanced(1, &vb, 36, 1, 0, 0);


	Thread::Sleep(14);

	graphics->EndFrame(true);
}

void RenderingSystem::PrevIteration()
{
	UpdateCurrentServer();
}

void RenderingSystem::PostIteration()
{
}


NAMESPACE_END