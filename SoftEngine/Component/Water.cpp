#include "Water.h"

#include <Common.h>

#include <Buffer.h>
#include <Resource.h>

#include <Engine/Engine.h>

GerstnerWavesWater::GerstnerWavesWater(uint32_t dimX, uint32_t dimY, float highUnit, float xUnit, float zUnit)
{
	m_numX = dimX;
	m_numY = dimY;

	SetUnit(highUnit, xUnit, zUnit);

	InitGPUBuffer(0);

	m_svar = new ShaderVar(&m_info, sizeof(Info));

	m_rpl = RenderPipelineManager::Get(
		R"(struct Vertex
		{
			Vec3 pos; POSITION, PER_VERTEX #
			Vec2 uv; TEXTCOORD, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		};)",
		L"HeightMap/Water/GerstnerWaves/VSWaves",
		L"HeightMap/Water/GerstnerWaves/PSWaves"
	);

	m_diffuse = Resource::Get<Texture2D>(L"D:/KEngine/ResourceFile/temp_img/Blue.png");
	m_normalMap = Resource::Get<Texture2D>(L"D:/KEngine/ResourceFile/temp_img/Water/normal3.png");

	m_material.ambient = { 0.1f, 0.1f, 0.1f };
	m_material.diffuse = { 1.f, 1.f, 1.f };
	m_material.shininess = 500;
	m_material.specular = 1.5f;
	m_materialSvar = new ShaderVar(&m_material, sizeof(SurfaceMaterial));

	m_fsType.type = ForwardShadingType::USE_NORMAL_MAP_AND_MATERIAL;
	m_fsTypeSvar = new ShaderVar(&m_fsType, sizeof(ForwardShadingType));
}

GerstnerWavesWater::~GerstnerWavesWater()
{
	Resource::Release(&m_diffuse);
	Resource::Release(&m_normalMap);

	delete m_vb;
	delete m_ib;

	delete m_svar;
	delete m_materialSvar;
	delete m_fsTypeSvar;
}

void GerstnerWavesWater::InitGPUBuffer(uint32_t uvMode)
{
	m_vb = nullptr;
	m_ib = nullptr;

	std::vector<Vertex> vb(m_numX * m_numY);
	std::vector<uint32_t> ib((m_numX - 1) * (m_numY - 1) * 6);

	uint32_t vbCount = 0;
	uint32_t ibCount = 0;

	//Vec2 uv[] = { {0,0}, {1,0}, {0,1}, {1,1} };
	//size_t c = 0;

	for (size_t i = 0; i < m_numY; i++)
	{
		for (size_t j = 0; j < m_numX; j++)
		{
			auto& v = vb[vbCount];
			v.pos.x = j * m_info.xUnit;
			v.pos.z = i * m_info.zUnit;
			v.pos.y = 0;

			v.normal = { 0,1,0 };
			//v.uv = uv[j % 2 + c];
			v.uv = { j / (float)m_numX,  i / (float)m_numY };

			vbCount++;
		}
		//c = (c + 2) % 4;
	}

	vbCount = 0;
	for (size_t i = 0; i < m_numY - 1; i++)
	{
		for (size_t j = 0; j < m_numX - 1; j++)
		{
			ib[ibCount] = vbCount;
			ib[ibCount + 1] = vbCount + 1;
			ib[ibCount + 2] = vbCount + m_numX + 1;

			ib[ibCount + 3] = vbCount + m_numX + 1;
			ib[ibCount + 4] = vbCount + m_numX;
			ib[ibCount + 5] = vbCount;

			vbCount++;
			ibCount += 6;
		}
		vbCount++;
	}

	m_vb = new VertexBuffer(&vb[0], vb.size(), sizeof(Vertex));
	m_ib = new IndexBuffer(&ib[0], ib.size(), sizeof(uint32_t));
}

void GerstnerWavesWater::Update(Engine* engine)
{
	//static float t = 0;
	m_info.t += engine->FDeltaTime();
}

void GerstnerWavesWater::Render(IRenderer* renderer)
{
	FlushTransform();
	m_svar->Update(&m_info, sizeof(m_info));
	m_rpl->VSSetVar(m_svar, 4);

	m_rpl->PSSetVar(m_materialSvar, 2);
	m_rpl->PSSetVar(m_fsTypeSvar, 3);
	m_rpl->PSSetResource(m_diffuse, FORWARD_LIGHTING_START_RESOURCE_SLOT);
	m_rpl->PSSetResource(m_normalMap, FORWARD_LIGHTING_START_RESOURCE_SLOT + 1);

	renderer->Render(m_rpl, m_vb, m_ib);

	//renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::LINE_LIST);
	//renderer->Render(m_rpl, m_vb, m_ib);
	//renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::TRIANGLE_LIST);
}
