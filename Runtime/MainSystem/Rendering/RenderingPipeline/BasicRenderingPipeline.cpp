#include "BasicRenderingPipeline.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"
#include "MainSystem/Rendering/Components/RENDER_TYPE.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "MainSystem/Rendering/BuiltinConstantBuffers.h"

#include "Graphics/DebugGraphics.h"

NAMESPACE_BEGIN

BasicSkyRenderingPass::BasicSkyRenderingPass()
{
	auto graphics = Graphics::Get();

	GRAPHICS_PIPELINE_DESC pipelineDesc = {};
	pipelineDesc.preferRenderCallPerFrame = 4096;
	pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.vs = "Sky/MieRayleigh.vs";
	pipelineDesc.ps = "Sky/MieRayleigh.ps";

	pipelineDesc.inputDesc.numElements = 0;

	pipelineDesc.outputDesc.numRenderTarget = 1;
	pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;

	m_pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
}

void BasicSkyRenderingPass::Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass)
{
	auto builtinCBs = pipeline->GetRenderingSystem()->GetBuiltinConstantBuffers();
	m_cameraBuffer = builtinCBs->GetCameraBuffer();
}

void BasicSkyRenderingPass::Run(RenderingPipeline* pipeline)
{
	auto graphics = Graphics::Get();
	auto output = pipeline->GetOutput();

	graphics->SetRenderTargets(1, &output, nullptr);

	graphics->ClearRenderTarget(output, { 0.0f, 0.0f, 0.0f, 0.0f }, 0, 0);
	graphics->SetGraphicsPipeline(m_pipeline.get());

	auto params = m_pipeline->PrepareRenderParams();
	params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_cameraBuffer);
	params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &m_cameraBuffer);
	graphics->DrawInstanced(0, 0, 6, 1, 0, 0);

	graphics->UnsetRenderTargets(1, &output, nullptr);
}

BasicAnimModelRenderingPass::BasicAnimModelRenderingPass()
{
	auto graphics = Graphics::Get();

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(Mat4) * MAX_BONES;
	cbDesc.perferNumRoom = 512;
	graphics->CreateConstantBuffers(1, &cbDesc, &m_bonesBuffer);

	{
		GRAPHICS_PIPELINE_DESC pipelineDesc = {};
		pipelineDesc.preferRenderCallPerFrame = 1024;
		pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.vs = "AnimModel/AnimModel4.vs";
		pipelineDesc.ps = "Test.ps";

		pipelineDesc.inputDesc.numElements = 7;
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
			"TANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[2] = {
			"BITANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 2,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[3] = {
			"NORMAL",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 3,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[4] = {
			"TEXTCOORD",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32_FLOAT,
			0,
			sizeof(Vec3) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[5] = {
			"BONE",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[4].alignedByteOffset + (uint32_t)sizeof(Vec2),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[6] = {
			"WEIGHT",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[5].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.outputDesc.numRenderTarget = 1;
		pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;

		m_animModel4Pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
	}

	{
		GRAPHICS_PIPELINE_DESC pipelineDesc = {};
		pipelineDesc.preferRenderCallPerFrame = 1024;
		pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.vs = "AnimModel/AnimModel8.vs";
		pipelineDesc.ps = "Test.ps";

		pipelineDesc.rasterizerDesc.cullMode = GRAPHICS_CULL_MODE::NONE;

		pipelineDesc.inputDesc.numElements = 9;
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
			"TANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[2] = {
			"BITANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 2,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[3] = {
			"NORMAL",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 3,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[4] = {
			"TEXTCOORD",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32_FLOAT,
			0,
			sizeof(Vec3) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[5] = {
			"BONE",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[4].alignedByteOffset + (uint32_t)sizeof(Vec2),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[6] = {
			"BONE",
			2,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[5].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[7] = {
			"WEIGHT",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[6].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[8] = {
			"WEIGHT",
			2,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[7].alignedByteOffset + (uint32_t)sizeof(Vec4),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.outputDesc.numRenderTarget = 1;
		pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;

		m_animModel8Pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
	}

	{
		GRAPHICS_PIPELINE_DESC pipelineDesc = {};
		pipelineDesc.preferRenderCallPerFrame = 1024;
		pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.vs = "AnimModel/AnimModel16.vs";
		pipelineDesc.ps = "Test.ps";

		pipelineDesc.inputDesc.numElements = 13;
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
			"TANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[2] = {
			"BITANGENT",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 2,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[3] = {
			"NORMAL",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(Vec3) * 3,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};
		pipelineDesc.inputDesc.elements[4] = {
			"TEXTCOORD",
			0,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32_FLOAT,
			0,
			sizeof(Vec3) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[5] = {
			"BONE",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[4].alignedByteOffset + (uint32_t)sizeof(Vec2),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[6] = {
			"BONE",
			2,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[5].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[7] = {
			"BONE",
			3,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[6].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[8] = {
			"BONE",
			4,
			GRAPHICS_DATA_FORMAT::FORMAT_R16G16B16A16_UINT,
			0,
			pipelineDesc.inputDesc.elements[7].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[9] = {
			"WEIGHT",
			1,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[8].alignedByteOffset + (uint32_t)sizeof(uint16_t) * 4,
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[10] = {
			"WEIGHT",
			2,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[9].alignedByteOffset + (uint32_t)sizeof(Vec4),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[11] = {
			"WEIGHT",
			3,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[10].alignedByteOffset + (uint32_t)sizeof(Vec4),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.inputDesc.elements[12] = {
			"WEIGHT",
			4,
			GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32A32_FLOAT,
			0,
			pipelineDesc.inputDesc.elements[11].alignedByteOffset + (uint32_t)sizeof(Vec4),
			GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		};

		pipelineDesc.outputDesc.numRenderTarget = 1;
		pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;

		m_animModel16Pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
	}
}

void BasicAnimModelRenderingPass::Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass)
{
	m_objectBuffer = ((BasicRenderingPipeline*)pipeline)->m_objectBuffer;
	m_cameraBuffer = ((BasicRenderingPipeline*)pipeline)->m_cameraBuffer;
}

void BasicAnimModelRenderingPass::Run(RenderingPipeline* pipeline)
{
	auto basicPipeline = (BasicRenderingPipeline*)pipeline;
	auto output = basicPipeline->GetOutput();

	auto depthBuffer = basicPipeline->GetOutputDepthBuffer();

	auto graphics = Graphics::Get();
	graphics->SetRenderTargets(1, &output, depthBuffer);

	//graphics->ClearDepthStencil(depthBuffer, 0, 0);
	
	Render(basicPipeline->m_animMesh4, m_animModel4Pipeline);
	Render(basicPipeline->m_animMesh8, m_animModel8Pipeline);
	Render(basicPipeline->m_animMesh16, m_animModel16Pipeline);

	graphics->UnsetRenderTargets(1, &output, depthBuffer);
}

void BasicAnimModelRenderingPass::Render(std::vector<AnimMeshRenderer*>& input, SharedPtr<GraphicsPipeline>& pipeline)
{
	auto graphics = Graphics::Get();

	graphics->SetGraphicsPipeline(pipeline.get());

	void* prevBuffer = nullptr;
	for (auto& comp : input)
	{
		//m_objectBuffer->UpdateBuffer(&comp->GlobalTransform(), sizeof(Mat4));

		graphics->GetDebugGraphics()->DrawAABox(comp->GetGlobalAABB());

		auto* shaderBuffer = comp->m_animMeshRenderingBuffer.get();
		if (prevBuffer != (void*)shaderBuffer)
		{
			auto buffer = shaderBuffer->buffer.Read();
			m_bonesBuffer->UpdateBuffer(buffer->bones.data(), buffer->bones.size() * sizeof(Mat4));
			prevBuffer = shaderBuffer;
		}

		auto params = pipeline->PrepareRenderParams();
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_cameraBuffer);
		//params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_objectBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_bonesBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &m_cameraBuffer);
		params->SetShaderResources(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &comp->GetTexture2D()->GetGraphicsShaderResource());

		auto vb = comp->GetMesh()->GetVertexBuffer().get();
		graphics->DrawInstanced(1, &vb, comp->GetMesh()->GetVertexCount(), 1, 0, 0);
	}
}

BasicRenderingPass::BasicRenderingPass()
{
	auto graphics = Graphics::Get();

	GRAPHICS_PIPELINE_DESC pipelineDesc = {};
	pipelineDesc.preferRenderCallPerFrame = 4096;
	pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.vs = "Test.vs";
	pipelineDesc.ps = "Test.ps";

	pipelineDesc.rasterizerDesc.cullMode = GRAPHICS_CULL_MODE::NONE;
	
	pipelineDesc.inputDesc.numElements = 5;
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
		"TANGENT",
		0,
		GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
		0,
		sizeof(Vec3),
		GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	};
	pipelineDesc.inputDesc.elements[2] = {
		"BITANGENT",
		0,
		GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
		0,
		sizeof(Vec3) * 2,
		GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	};
	pipelineDesc.inputDesc.elements[3] = {
		"NORMAL",
		0,
		GRAPHICS_DATA_FORMAT::FORMAT_R32G32B32_FLOAT,
		0,
		sizeof(Vec3) * 3,
		GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	};
	pipelineDesc.inputDesc.elements[4] = {
		"TEXTCOORD",
		0,
		GRAPHICS_DATA_FORMAT::FORMAT_R32G32_FLOAT,
		0,
		sizeof(Vec3) * 4,
		GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	};

	pipelineDesc.outputDesc.numRenderTarget = 1;
	pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;

	m_pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
}

void BasicRenderingPass::Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass)
{
	auto graphics = Graphics::Get();
	auto output = pipeline->GetOutput();

	m_depthBuffer = pipeline->GetOutputDepthBuffer();

	auto builtinCBs = pipeline->GetRenderingSystem()->GetBuiltinConstantBuffers();
	m_cameraBuffer = builtinCBs->GetCameraBuffer();

	m_objectBuffer = ((BasicRenderingPipeline*)pipeline)->m_objectBuffer;
}

void BasicRenderingPass::Run(RenderingPipeline* pipeline)
{
	assert(dynamic_cast<BasicRenderingPipeline*>(pipeline));

	auto basicPipeline = (BasicRenderingPipeline*)pipeline;
	auto& input = basicPipeline->m_basicModel3Ds;
	auto output = basicPipeline->GetOutput();

	auto graphics = Graphics::Get();
	graphics->SetRenderTargets(1, &output, m_depthBuffer);

	//graphics->ClearRenderTarget(output, { 0.0f, 0.0f, 0.0f, 0.0f }, 0, 0);
	graphics->ClearDepthStencil(m_depthBuffer, 0, 0);
	graphics->SetGraphicsPipeline(m_pipeline.get());

	//assert(input.size() == 2);

	//static size_t count = 0;
	////std::cout << ++count << "\n";
	//++count;
	//if (count >= 86)
	//{
	//	int x = 3;
	//}

//#ifdef _DEBUG
//	auto debugGraphics = graphics->GetDebugGraphics();
//#endif // _DEBUG

	for (auto& comp : input)
	{
		auto model = (MeshBasicRenderer*)comp;

//#ifdef _DEBUG
//		debugGraphics->DrawAABox(model->GetGlobalAABB());
//
//		auto& mat = comp->GlobalTransform();
//		debugGraphics->DrawDirection(mat.Position(), mat.Forward());
//#endif // _DEBUG

		m_objectBuffer->UpdateBuffer(&model->GlobalTransform(), sizeof(Mat4));

		auto params = m_pipeline->PrepareRenderParams();
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_cameraBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_objectBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &m_cameraBuffer);
		params->SetShaderResources(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &model->GetTexture2D()->GetGraphicsShaderResource());

		auto vb = model->GetMesh()->GetVertexBuffer().get();
		graphics->DrawInstanced(1, &vb, model->GetMesh()->GetVertexCount(), 1, 0, 0);
	}

	graphics->UnsetRenderTargets(1, &output, m_depthBuffer);
}

BasicRenderingPipeline::BasicRenderingPipeline()
{
	auto graphics = Graphics::Get();

	m_objectData.transform = Mat4::Identity();

	m_cameraBuffer = BuiltinConstantBuffers::Get()->GetCameraBuffer();

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(ObjectData);
	cbDesc.perferNumRoom = 4096;
	graphics->CreateConstantBuffers(1, &cbDesc, &m_objectBuffer);

	AddRenderingPass(new BasicSkyRenderingPass());
	AddRenderingPass(new BasicRenderingPass());
	AddRenderingPass(new BasicAnimModelRenderingPass());
}

void BasicRenderingPipeline::SetInput(RenderingComponent** components, size_t count)
{
	m_input = components;
	m_inputCount = count;

	m_basicModel3Ds.clear();
	m_animMesh4.clear();
	m_animMesh8.clear();
	m_animMesh16.clear();

	for (size_t i = 0; i < count; i++)
	{
		auto comp = components[i];
		if (comp->GetRenderType() == RENDER_TYPE_MESH_BASIC_RENDERER)
		{
			m_basicModel3Ds.push_back(comp);
			continue;
		}

		if (comp->GetRenderType() == RENDER_TYPE_ANIM_MESH_RENDERER)
		{
			auto animMeshRenderer = (AnimMeshRenderer*)comp;
			auto type = animMeshRenderer->m_mesh->m_type;

			switch (type)
			{
			case soft::AnimModel::WEIGHT_4:
				m_animMesh4.push_back(animMeshRenderer);
				break;
			case soft::AnimModel::WEIGHT_8:
				m_animMesh8.push_back(animMeshRenderer);
				break;
			case soft::AnimModel::WEIGHT_16:
				m_animMesh16.push_back(animMeshRenderer);
				break;
			default:
				break;
			}

			continue;
		}
	}

	// sort anim mesh to reduce GPU bones constant buffer upload calls
	std::sort(m_animMesh4.begin(), m_animMesh4.end(),
		[](AnimMeshRenderer* a, AnimMeshRenderer* b)
		{
			return (void*)a->m_animMeshRenderingBuffer.get() > (void*)b->m_animMeshRenderingBuffer.get();
		}
	);

	std::sort(m_animMesh8.begin(), m_animMesh8.end(),
		[](AnimMeshRenderer* a, AnimMeshRenderer* b)
		{
			return (void*)a->m_animMeshRenderingBuffer.get() > (void*)b->m_animMeshRenderingBuffer.get();
		}
	);

	std::sort(m_animMesh16.begin(), m_animMesh16.end(),
		[](AnimMeshRenderer* a, AnimMeshRenderer* b)
		{
			return (void*)a->m_animMeshRenderingBuffer.get() > (void*)b->m_animMeshRenderingBuffer.get();
		}
	);
}

NAMESPACE_END