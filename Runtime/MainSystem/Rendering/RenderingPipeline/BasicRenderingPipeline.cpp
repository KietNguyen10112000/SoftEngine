#include "BasicRenderingPipeline.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/RENDER_TYPE.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#ifdef _DEBUG
#include "Graphics/DebugGraphics.h"
#endif // _DEBUG

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

BasicRenderingPass::BasicRenderingPass()
{
	auto graphics = Graphics::Get();

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(ObjectData);
	cbDesc.perferNumRoom = 4096;
	graphics->CreateConstantBuffers(1, &cbDesc, &m_objectBuffer);

	GRAPHICS_PIPELINE_DESC pipelineDesc = {};
	pipelineDesc.preferRenderCallPerFrame = 4096;
	pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.vs = "Test.vs";
	pipelineDesc.ps = "Test.ps";

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

	m_objectData.transform = Mat4::Identity();
}

void BasicRenderingPass::Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass)
{
	auto graphics = Graphics::Get();
	auto output = pipeline->GetOutput();

	m_depthBuffer = pipeline->GetOutputDepthBuffer();

	auto builtinCBs = pipeline->GetRenderingSystem()->GetBuiltinConstantBuffers();
	m_cameraBuffer = builtinCBs->GetCameraBuffer();
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
	AddRenderingPass(new BasicSkyRenderingPass());
	AddRenderingPass(new BasicRenderingPass());
}

void BasicRenderingPipeline::SetInput(RenderingComponent** components, size_t count)
{
	m_input = components;
	m_inputCount = count;

	m_basicModel3Ds.clear();
	for (size_t i = 0; i < count; i++)
	{
		auto comp = components[i];
		if (comp->GetRenderType() == RENDER_TYPE_MESH_BASIC_RENDERER)
		{
			m_basicModel3Ds.push_back(comp);
		}
	}
}

NAMESPACE_END