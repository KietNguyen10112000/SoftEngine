#include "HeightMap.h"

#define STB_IMAGE_IMPLEMENTATION
#include <Libs/stb/stb_image.h>

#include <Common.h>

#include <Buffer.h>
#include <Resource.h>


HeightMap::HeightMap(uint32_t dimX, uint32_t dimY)
{
}

HeightMap::HeightMap(const std::wstring& path, const std::wstring& colorMapPath, uint32_t uvMode,
	float highUnit, float xUnit, float zUnit)
{
	SetUnit(highUnit, xUnit, zUnit);

	LoadFromImage(path, colorMapPath, uvMode);

	m_svar = new ShaderVar(&m_info, sizeof(Info));

	m_rpl = RenderPipelineManager::Get(
		R"(struct Vertex
		{
			Vec3 pos; POSITION, PER_VERTEX #
			Vec2 uv; TEXTCOORD, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		};)", 
		L"HeightMap/VSHeightMap", 
		L"HeightMap/PSHeightMap"
	);
}

HeightMap::~HeightMap()
{
	Resource::Release(&m_diffuse);

	delete m_vb;
	delete m_ib;
	delete m_svar;
}

void HeightMap::ResizeGrid(uint32_t newX, uint32_t newY)
{
	m_numX = newX;
	m_numY = newY;

//#ifdef _DEBUG
	m_height.resize(newY);
	for (size_t i = 0; i < newY; i++)
	{
		m_height[i].resize(newX, 0);
	}
//#else
//	static_assert(0);
//#endif // _DEBUG

}

void HeightMap::InitGPUBuffer(uint32_t uvMode)
{
	delete m_vb;
	delete m_ib;

	m_vb = nullptr;
	m_ib = nullptr;

	std::vector<Vertex> vb(m_numX * m_numY);
	std::vector<uint32_t> ib((m_numX - 1) * (m_numY - 1) * 6);

	uint32_t vbCount = 0;
	uint32_t ibCount = 0;

	for (size_t i = 0; i < m_numY; i++)
	{
		for (size_t j = 0; j < m_numX; j++)
		{
			auto& v = vb[vbCount];
			v.pos.x = j * m_info.xUnit;
			v.pos.z = i * m_info.zUnit;
			v.pos.y = GetHeight(i, j);

			v.normal = GetNormal(i, j);

			vbCount++;
		}
	}

	vbCount = 0;
	for (size_t i = 0; i < m_numY - 1; i++)
	{
		for (size_t j = 0; j < m_numX - 1; j++)
		{
			ib[ibCount]		= vbCount;
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

bool HeightMap::LoadFromImage(const std::wstring& heightMapPath, const std::wstring& colorMapPath, uint32_t uvMode)
{
	int nrChannels, imgWidth, imgHeight;
	unsigned char* imgData = stbi_load(WStringToString(heightMapPath).c_str(), &imgWidth, &imgHeight, &nrChannels, STBI_grey);

	ResizeGrid(imgWidth, imgHeight);

	for (size_t i = 0; i < m_numY; i++)
	{
		for (size_t j = 0; j < m_numX; j++)
		{
			m_height[i][j] = imgData[RowMajorArrayIndex2dTo1d(i, j, m_numX)] / 255.0f - 0.5f;
		}
	}

	InitGPUBuffer(uvMode);

	if (!colorMapPath.empty())
	{
		Resource::Release(&m_diffuse);
		m_diffuse = Resource::Get<Texture2D>(colorMapPath);
	}

	return true;
}

void HeightMap::SetUnit(float highUnit, float xUnit, float zUnit)
{
	m_info = { highUnit, xUnit, zUnit, 0 };
}

Vec3 HeightMap::GetNormal(uint32_t x, uint32_t y)
{
	float hL = 0.5f;
	float hR = 0.5f;
	float hD = 0.5f;
	float hU = 0.5f;

	if (y >= 1 && y < m_numY - 1)
	{
		hU = GetHeight(x, y - 1);
		hD = GetHeight(x, y + 1);
	}

	if (x >= 1 && x < m_numX - 1)
	{
		hL = GetHeight(x - 1, y);
		hR = GetHeight(x + 1, y);
	}

	return Vec3(hL - hR, 2.0f, hD - hU).Normalize();
}


void HeightMap::Update(Engine* engine)
{

}

void HeightMap::Render(IRenderer* renderer)
{
	FlushTransform();
	m_svar->Update(&m_info, sizeof(m_info));
	m_rpl->VSSetVar(m_svar, 4);
	m_rpl->PSSetResource(m_diffuse, 0);

	//renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::LINE_LIST);
	renderer->Render(m_rpl, m_vb, m_ib);
	//renderer->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY::TRIANGLE_LIST);
}