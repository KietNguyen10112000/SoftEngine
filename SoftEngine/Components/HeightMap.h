#pragma once

#include <IObject.h>

//CPU side height map
class HeightMap : public IRenderableObject
{
//for render pass
protected:
	struct Vertex
	{
		Vec3 pos;
		Vec2 uv = { 0,0 };
		Vec3 normal;
	};

	VertexBuffer* m_vb = nullptr;
	IndexBuffer* m_ib = nullptr;

	Texture2D* m_diffuse = nullptr;

	//info constant buffer
	ShaderVar* m_svar = nullptr;

public:
	struct Info
	{
		float highUnit = 0;
		float xUnit = 0;
		float zUnit = 0;
		float padding;
	};

//for logic pass
protected:
	//dimension of grid
	uint32_t m_numX = 0;
	uint32_t m_numY = 0;

	Info m_info;


//#ifdef _DEBUG
	std::vector<std::vector<float>> m_height;
//#else
//	float** m_height = nullptr;
//#endif // _DEBUG

public:
	HeightMap(uint32_t dimX, uint32_t dimY);
	HeightMap(const std::wstring& path, const std::wstring& colorMapPath, uint32_t uvMode,
		float highUnit, float xUnit, float zUnit);

	virtual ~HeightMap();

protected:
	void ResizeGrid(uint32_t newX, uint32_t newY); 
	void InitGPUBuffer(uint32_t uvMode);

	void Clear();

public:
	bool LoadFromImage(const std::wstring& path, const std::wstring& colorMapPath, uint32_t uvMode);

	void SetUnit(float highUnit, float xUnit, float zUnit);

public:
	inline float GetHeight(uint32_t x, uint32_t y) { if (x >= m_numX || y >= m_numY) return 0; return m_height[y][x] * m_info.highUnit; };
	Vec3 GetNormal(uint32_t x, uint32_t y);

public:
	virtual void Update(Engine* engine) override;
	virtual void Render(IRenderer* renderer) override;

};
