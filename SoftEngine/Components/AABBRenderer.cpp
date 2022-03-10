#include "AABBRenderer.h"

#include "Core/Buffer.h"

#include "Core/RenderPipeline.h"

#include "Core/IObject.h"

AABBRenderer::AABBRenderer()
{
	m_aabbs.reserve(MAX_AABB_PER_FRAME);
	m_aabbs.resize(MAX_AABB_PER_FRAME);

	m_vb = new VertexBuffer(m_aabbs.data(), m_aabbs.size(), sizeof(AABB), 2);

	m_rpl = RenderPipelineManager::Get(
		R"(
			struct Input
			{
				Vec3 pos; POSITION, PER_VERTEX #
				Vec3 dim; DIMENSIONS, PER_VERTEX #
			};
		)",
		L"AABBRenderer/AABBRenderer_VS", 
		L"AABBRenderer/AABBRenderer_GS", 
		L"AABBRenderer/AABBRenderer_PS"
	);
	m_aabbs.resize(0);
}

AABBRenderer::~AABBRenderer()
{
	delete m_vb;
	RenderPipelineManager::Release(&m_rpl);
}

void AABBRenderer::Present(IRenderer* renderer)
{
	m_vb->Update(m_aabbs.data(), sizeof(AABB) * m_aabbs.size());
	m_vb->Count() = m_aabbs.size();
	renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::POINT_LIST);
	RenderPipeline::GSSetVar(ICamera::shaderMVP, 1);
	renderer->Render(m_rpl, m_vb);
	renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::TRIANGLE_LIST);
	m_aabbs.clear();
}

void AABBRenderer::Add(const AABB& aabb)
{
	if (!aabb.IsValid()) return;

	if (m_aabbs.size() >= MAX_AABB_PER_FRAME) return;

	m_aabbs.push_back(aabb);
}
