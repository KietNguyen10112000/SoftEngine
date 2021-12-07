#pragma once

#include <IObject.h>

#include <Buffer.h>

#include <vector>

class SpaceCoordinate : public IRenderableObject
{
private:
	VertexBuffer* m_pVertexBuffer = nullptr;
	VertexBuffer* m_pGridVertexBuffer = nullptr;

	int m_numGridVertexs = 0;

	bool m_isDisplayGrid = true;

	bool m_isRender = true;

public:
	inline SpaceCoordinate();
	inline ~SpaceCoordinate();

public:
	// Inherited via IRenderableObject
	inline virtual void Update(Engine* engine) override;
	inline virtual void Render(IRenderer* renderer) override;

public:
	inline auto& DisplayGrid() { return m_isDisplayGrid; };
	inline auto& WillRender() { return m_isRender; };

};

inline SpaceCoordinate::SpaceCoordinate()
{
	struct SpaceVertex
	{
		Vec3 pos;
		Vec4 color;
	};

	SpaceVertex vertexs[] = {
		{Vec3(10000.0f, 0, 0),		Vec4(1, 0, 0, 1)}, //x-axis
		{Vec3(-10000.0f, 0, 0),		Vec4(1, 0, 0, 1)},
		{Vec3(0, 10000.0f, 0),		Vec4(0, 1, 0, 1)}, //y-axis
		{Vec3(0, -10000.0f, 0),		Vec4(0, 1, 0, 1)},
		{Vec3(0, 0, 10000.0f),		Vec4(0, 0, 1, 1)}, //z-axis
		{Vec3(0, 0, -10000.0f),		Vec4(0, 0, 1, 1)},
	};

	m_pVertexBuffer = new VertexBuffer(vertexs, sizeof(vertexs) / sizeof(SpaceVertex), sizeof(SpaceVertex));

	std::vector<SpaceVertex> gridVertexs(1000);

	m_numGridVertexs = gridVertexs.size();
	float dX = 10.0f;
	float dZ = 10.0f;
	float x = -(m_numGridVertexs / 4) * dX / 2;
	float z = -(m_numGridVertexs / 4) * dZ / 2;
	//float alpha = 0.5f;
	//float grayScale = 0.9f;

	//Vec3 color = { 1, 1, 1 };

	Vec4 color = { 0, 0.3, 0.5, 0.5 };

	for (size_t i = 0; i < m_numGridVertexs / 2; i += 2)
	{
		if (x != 0)
		{
			gridVertexs[i] = { Vec3(x, 0, 10000.0f),	color };
			gridVertexs[i + 1] = { Vec3(x, 0, -10000.0f), color };
		}

		x += dX;
	}
	for (size_t i = m_numGridVertexs / 2; i < m_numGridVertexs; i += 2)
	{
		if (z != 0)
		{
			gridVertexs[i] = { Vec3(10000.0f, 0, z),	color };
			gridVertexs[i + 1] = { Vec3(-10000.0f, 0, z), color };
		}

		z += dZ;
	}

	m_pGridVertexBuffer = new VertexBuffer(&gridVertexs[0], m_numGridVertexs, sizeof(SpaceVertex));

	m_rpl = RenderPipelineManager::Get(
		R"(struct Vertex
		{
			Vec3 pos; POSITION, PER_VERTEX #
			Vec4 color; COLOR, PER_VERTEX #
		};)",
		L"VSSpaceCoordinate",
		L"PSSpaceCoordinate"
	);
}

inline SpaceCoordinate::~SpaceCoordinate()
{
	delete m_pGridVertexBuffer;
	delete m_pVertexBuffer;
}

inline void SpaceCoordinate::Update(Engine* engine)
{
}

inline void SpaceCoordinate::Render(IRenderer* renderer)
{
	if (!m_isRender) return;

	FlushTransform();

	renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::LINE_LIST);

	if(m_isDisplayGrid)
		renderer->Render(m_rpl, m_pGridVertexBuffer);

	renderer->Render(m_rpl, m_pVertexBuffer);

	renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::TRIANGLE_LIST);
}
