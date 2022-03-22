#pragma once

#include <IResource.h>

//#include <Math/Math.h>

#include "Math/AABB.h"

#include <Buffer.h>

#include <vector>

//#define KEEP_MESH_DATA 1
#define BASIC_MODEL_KEEP_MESH_MATERIAL 1
#define TBN_MODEL_KEEP_MESH_MATERIAL 1
//#define STATIC_MODEL3D_THROW_IF_ANIMATED 1
//#define KEEP_MESH_NAME 1
//#define BASIC_MODEL_UNIQUE_SHADER_VAR 1

#define MESH_LOCAL_TRANSFORM_LOCATION 2
#define MESH_PS_MATERIAL_LOCATION 2

#ifdef KEEP_MESH_NAME
#include <string>
#endif // KEEP_MESH_NAME

#ifdef BASIC_MODEL_UNIQUE_SHADER_VAR
class ShaderVar;
#endif

class IRenderer;
class RenderPipeline;

//static basic model
//args[0] is per mesh pre-transformation
class BasicModel : public IResource
{
private:
	friend class ModelLoaderHelperFunc;

#ifdef BASIC_MODEL_UNIQUE_SHADER_VAR
public:
	ShaderVar* localTransformShaderVar = nullptr;
	ShaderVar* materialShaderVar = nullptr;
	uint32_t count = 0;

#endif

public:
	struct Vertex
	{
		Vec3 position = {};
		Vec2 textCoord = {};
		Vec3 normal = {};
	};

	struct MeshMaterial
	{
		Vec3 ambient = {};
		float specular = 1;
		Vec3 diffuse = {};
		float shininess = 0;
	};

	struct Mesh
	{
		AABB aabb;
		VertexBuffer* vb = nullptr;
		IndexBuffer* ib = nullptr;


#ifndef BASIC_MODEL_UNIQUE_SHADER_VAR
		ShaderVar* svar = nullptr;
		ShaderVar* materialSvar = nullptr;
#else

#ifndef KEEP_MESH_DATA
		Mat4x4 localTransform;
		MeshMaterial material;
#endif

#endif

#ifndef KEEP_MESH_DATA
#ifdef	BASIC_MODEL_KEEP_MESH_MATERIAL
		MeshMaterial material;
#endif
#endif

#ifdef KEEP_MESH_DATA
		Mat4x4 localTransform;
		MeshMaterial material;
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
#endif // KEEP_MESH_DATA


#ifdef KEEP_MESH_NAME
		std::wstring name;
#endif // KEEP_MESH_NAME

	};


public:
	std::vector<Mesh> m_meshs;

public:
	BasicModel(const std::wstring& path, uint32_t numArg, void** args);
	~BasicModel();

public:
	void Render(IRenderer* renderer, RenderPipeline* rpl);

	//count will be number of mesh
	inline Mesh* GetMeshs(size_t& count) { count = m_meshs.size(); return &m_meshs[0]; }

	inline auto& Meshs() { return m_meshs; };

};

//static model with calculated TBN matrix
//args[0] is per mesh pre-transformation
class TBNModel : public IResource
{
private:
	friend class ModelLoaderHelperFunc;

#ifdef TBN_MODEL_UNIQUE_SHADER_VAR
public:
	ShaderVar* localTransformShaderVar = nullptr;
	ShaderVar* materialShaderVar = nullptr;
	uint32_t count = 0;
#endif

public:
	struct Vertex
	{
		Vec3 position = {};
		Vec2 textCoord = {};
		Vec3 tangent = {};
		Vec3 bitangent = {};
		Vec3 normal = {};
	};

	struct MeshMaterial
	{
		Vec3 ambient = {};
		float specular = 1;
		Vec3 diffuse = {};
		float shininess = 0;
	};

	struct Mesh
	{
		AABB aabb;
		VertexBuffer* vb = nullptr;
		IndexBuffer* ib = nullptr;


#ifndef TBN_MODEL_UNIQUE_SHADER_VAR
		ShaderVar* svar = nullptr;
		ShaderVar* materialSvar = nullptr;
#else

#ifndef KEEP_MESH_DATA
		Mat4x4 localTransform;
		MeshMaterial material;
#endif

#endif

#ifndef KEEP_MESH_DATA
#ifdef	TBN_MODEL_KEEP_MESH_MATERIAL
		MeshMaterial material;
#endif
#endif

#ifdef KEEP_MESH_DATA
		Mat4x4 localTransform;
		MeshMaterial material;
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
#endif // KEEP_MESH_DATA


#ifdef KEEP_MESH_NAME
		std::wstring name;
#endif // KEEP_MESH_NAME

	};


private:
	std::vector<Mesh> m_meshs;

public:
	TBNModel(const std::wstring& path, uint32_t numArg, void** args);
	~TBNModel();

public:
	void Render(IRenderer* renderer, RenderPipeline* rpl);

	//count will be number of mesh
	inline Mesh* GetMeshs(size_t& count) { count = m_meshs.size(); return &m_meshs[0]; }

	inline auto& Meshs() { return m_meshs; };

};

//non material constant buffer
class RawTBNModel : public IResource
{
private:
	friend class ModelLoaderHelperFunc;

public:
	struct Vertex
	{
		Vec3 position = {};
		Vec2 textCoord = {};
		Vec3 tangent = {};
		Vec3 bitangent = {};
		Vec3 normal = {};
	};

	struct Mesh
	{
		AABB aabb;
		VertexBuffer* vb = nullptr;
		IndexBuffer* ib = nullptr;

		ShaderVar* svar = nullptr;

#ifdef KEEP_MESH_NAME
		std::wstring name;
#endif // KEEP_MESH_NAME
	};

public:
	std::vector<RawTBNModel::Mesh> m_meshs;

public:
	RawTBNModel(const std::wstring& path, uint32_t numArg, void** args);
	~RawTBNModel();

public:
	void Render(IRenderer* renderer, RenderPipeline* rpl);

	//count will be number of mesh
	inline Mesh* GetMeshs(size_t& count) { count = m_meshs.size(); return &m_meshs[0]; }

	inline auto& Meshs() { return m_meshs; };

};