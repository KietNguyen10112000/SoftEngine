#include "BasicRenderingPipeline.h"

#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"
#include "MainSystem/Rendering/Components/RENDER_TYPE.h"
#include "MainSystem/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

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
		"TEXTCOORD",
		0,
		GRAPHICS_DATA_FORMAT::FORMAT_R32G32_FLOAT,
		0,
		sizeof(Vec3),
		GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	};

	pipelineDesc.outputDesc.numRenderTarget = 1;
	pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;

	m_pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);

	m_objectData.transform = Mat4::Identity();
}

void BasicRenderingPass::Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass)
{
	auto graphics = Graphics::Get();
	auto output = pipeline->GetOutput();

	GRAPHICS_SHADER_RESOURCE_DESC outputDesc = {};
	output->GetShaderResource()->GetDesc(&outputDesc);

	// create depth buffer
	GRAPHICS_DEPTH_STENCIL_BUFFER_DESC depthBufferDesc = {};
	depthBufferDesc.format = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;
	depthBufferDesc.mipLevels = 1;
	depthBufferDesc.width = outputDesc.texture2D.width;
	depthBufferDesc.height = outputDesc.texture2D.height;
	m_depthBuffer = graphics->CreateDepthStencilBuffer(depthBufferDesc);

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
	graphics->SetRenderTargets(1, &output, m_depthBuffer.get());

	graphics->ClearRenderTarget(output, { 0.0f, 0.0f, 0.0f, 0.0f }, 0, 0);
	graphics->ClearDepthStencil(m_depthBuffer.get(), 0, 0);
	graphics->SetGraphicsPipeline(m_pipeline.get());

	//assert(input.size() == 2);

	//static size_t count = 0;
	////std::cout << ++count << "\n";
	//++count;
	//if (count >= 86)
	//{
	//	int x = 3;
	//}

	for (auto& comp : input)
	{
		auto model = (Model3DBasicRenderer*)comp;

		m_objectBuffer->UpdateBuffer(&model->GlobalTransform(), sizeof(Mat4));

		auto params = m_pipeline->PrepareRenderParams();
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_cameraBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &m_objectBuffer);
		params->SetShaderResources(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &model->GetTexture2D()->GetGraphicsShaderResource());

		auto vb = model->GetModel()->GetVertexBuffer().get();
		graphics->DrawInstanced(1, &vb, model->GetModel()->GetVertexCount(), 1, 0, 0);
	}

	graphics->UnsetRenderTargets(1, &output, m_depthBuffer.get());
}

BasicRenderingPipeline::BasicRenderingPipeline()
{
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
		if (comp->GetRenderType() == RENDER_TYPE_MODEL3D_BASIC_RENDERER)
		{
			m_basicModel3Ds.push_back(comp);
		}
	}
}

NAMESPACE_END