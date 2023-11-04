#include "DisplayService.h"

NAMESPACE_BEGIN

DisplayService::DisplayService()
{
	auto graphics = Graphics::Get();

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(CBuffer);
	cbDesc.perferNumRoom = 128;
	graphics->CreateConstantBuffers(1, &cbDesc, &m_constantBuffer);

	GRAPHICS_PIPELINE_DESC pipelineDesc = {};
	pipelineDesc.preferRenderCallPerFrame = 128;
	pipelineDesc.primitiveTopology = GRAPHICS_PRIMITIVE_TOPOLOGY::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.vs = "DisplayService/Quad.vs";
	pipelineDesc.ps = "DisplayService/Quad.ps";

	pipelineDesc.inputDesc.numElements = 0;
	
	pipelineDesc.outputDesc.numRenderTarget = 1;
	pipelineDesc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;

	m_pipeline = graphics->CreateRasterizerPipeline(pipelineDesc);
}

void DisplayService::Begin()
{
	auto graphics = Graphics::Get();

	auto screenRT = graphics->GetScreenRenderTarget();
	auto screenDS = graphics->GetScreenDepthStencilBuffer();

	graphics->SetRenderTargets(1, &screenRT, screenDS);
	graphics->ClearRenderTarget(screenRT, { 0.1f, 0.5f, 0.5f, 1.0f }, 0, 0);
	graphics->ClearDepthStencil(screenDS, 0, 0);
}

void DisplayService::End()
{
	auto graphics = Graphics::Get();

	auto screenRT = graphics->GetScreenRenderTarget();
	auto screenDS = graphics->GetScreenDepthStencilBuffer();

	graphics->UnsetRenderTargets(1, &screenRT, screenDS);
}

void DisplayService::Display(SharedPtr<GraphicsShaderResource>& resource, GRAPHICS_VIEWPORT viewport)
{
	auto graphics = Graphics::Get();
	auto baseDims = Vec2(graphics->GetWindowWidth(), graphics->GetWindowHeight());

	m_cbuffer.vertices[0].xy() = (viewport.topLeft);
	m_cbuffer.vertices[1].xy() = (viewport.topLeft + Vec2(viewport.size.x, 0));
	m_cbuffer.vertices[2].xy() = (viewport.topLeft + Vec2(0, viewport.size.y));

	m_cbuffer.vertices[3].xy() = (viewport.topLeft + Vec2(viewport.size.x, 0));
	m_cbuffer.vertices[4].xy() = (viewport.topLeft + Vec2(viewport.size.x, viewport.size.y));
	m_cbuffer.vertices[5].xy() = (viewport.topLeft + Vec2(0, viewport.size.y));

	for (size_t i = 0; i < 6; i++)
	{
		auto& v = m_cbuffer.vertices[i];
		v.xy() /= baseDims;
		v.y = 1 - v.y;
		v.xy() = (v.xy() - 0.5f) * 2.0f;
	}

	m_constantBuffer->UpdateBuffer(&m_cbuffer, sizeof(m_cbuffer));

	auto params = m_pipeline->PrepareRenderParams();
	params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &m_constantBuffer);
	params->SetShaderResources(GRAPHICS_SHADER_SPACE::SHADER_SPACE_PS, 0, 1, &resource);
	graphics->SetGraphicsPipeline(m_pipeline.get());
	graphics->DrawInstanced(0, 0, 6, 1, 0, 0);
}

NAMESPACE_END