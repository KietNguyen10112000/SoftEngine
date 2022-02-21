#pragma once

#include <IObject.h>

class GerstnerWavesWater : public IRenderableObject
{
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
	Texture2D* m_normalMap = nullptr;

	//info constant buffer
	ShaderVar* m_svar = nullptr;

	struct SurfaceMaterial
	{
		Vec3 ambient = {};
		float specular = 1;
		Vec3 diffuse = {};
		float shininess = 0;
	};

	SurfaceMaterial m_material = {};
	ShaderVar* m_materialSvar = nullptr;

	ForwardShadingType m_fsType = {};
	ShaderVar* m_fsTypeSvar = nullptr;

public:
	struct Info
	{
		float highUnit = 0;
		float xUnit = 0;
		float zUnit = 0;
		float t = 0;
	};

protected:
	//dimension of grid
	uint32_t m_numX = 0;
	uint32_t m_numY = 0;

	Info m_info;

	//uint32_t m_numWave = 1;

public:
	GerstnerWavesWater(uint32_t dimX, uint32_t dimY, float highUnit, float xUnit, float zUnit);
	virtual ~GerstnerWavesWater();

protected:
	void InitGPUBuffer(uint32_t uvMode);

	inline void SetUnit(float highUnit, float xUnit, float zUnit)
	{
		m_info = { highUnit, xUnit, zUnit, 0 };
	}

public:
	virtual void Update(Engine* engine) override;
	virtual void Render(IRenderer* renderer) override;

};
