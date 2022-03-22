#include "StaticObject.h"

#include <Resource.h>
#include <RenderPipeline.h>

#include <Model3D.h>

BasicObject::BasicObject(const std::wstring& modelPath, const std::wstring& texturePath, const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<BasicModel>(modelPath, 1, args);
	m_diffuse = Resource::Get<Texture2D>(texturePath);

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VSBasicModel",
		L"Model3D/PSBasicModel"
	);

}

BasicObject::~BasicObject()
{
	Resource::Release(&m_model);
	Resource::Release(&m_diffuse);
}

void BasicObject::Render(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResource(m_diffuse, 0);
	m_model->Render(renderer, m_rpl);
}

void BasicObject::RenderShadow(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResource(m_diffuse, 0);
	
	for (auto& mesh : m_model->Meshs())
	{
		m_rpl->VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		m_rpl->PSSetVar(mesh.materialSvar, MESH_PS_MATERIAL_LOCATION);
		renderer->RenderShadow(mesh.vb, mesh.ib);
	}
}

NormalMappingObject::NormalMappingObject(const std::wstring& modelPath, 
	const std::wstring& diffuseMapPath, const std::wstring& normalMapPath,
	const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<TBNModel>(modelPath, 1, args);

	size_t flag = Texture2D::FLAG::DISCARD_SRGB;
	args[0] = &flag;

	m_diffuseMap = Resource::Get<Texture2D>(diffuseMapPath, 1, args);
	m_normalMap = Resource::Get<Texture2D>(normalMapPath, 1, args);

	m_resourceHandles[0] = m_diffuseMap->GetNativeHandle();
	m_resourceHandles[1] = m_normalMap->GetNativeHandle();

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VS_TBN_Model",
		L"Model3D/PSNormalMapping"
	);

}

NormalMappingObject::~NormalMappingObject()
{
	Resource::Release(&m_model);
	Resource::Release(&m_diffuseMap);
	Resource::Release(&m_normalMap);
}

void NormalMappingObject::Render(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResources(m_resourceHandles, 2, 0);
	m_model->Render(renderer, m_rpl);
}



NormalSpecularMappingObject::NormalSpecularMappingObject(
	const std::wstring& modelPath,
	const std::wstring& diffuseMapPath, 
	const std::wstring& normalMapPath, 
	const std::wstring& specularMapPath,
	const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<TBNModel>(modelPath, 1, args);

	size_t flag = Texture2D::FLAG::DISCARD_SRGB;
	args[0] = &flag;

	m_diffuseMap = Resource::Get<Texture2D>(diffuseMapPath, 1, args);
	m_normalMap = Resource::Get<Texture2D>(normalMapPath, 1, args);
	m_specularMap = Resource::Get<Texture2D>(specularMapPath, 1, args);

	m_resourceHandles[0] = m_diffuseMap->GetNativeHandle();
	m_resourceHandles[1] = m_normalMap->GetNativeHandle();
	m_resourceHandles[2] = m_specularMap->GetNativeHandle();

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VS_TBN_Model",
		L"Model3D/PSNormalSpecularMapping"
	);

}

NormalSpecularMappingObject::~NormalSpecularMappingObject()
{
	Resource::Release(&m_model);
	Resource::Release(&m_diffuseMap);
	Resource::Release(&m_normalMap);
	Resource::Release(&m_specularMap);
}

void NormalSpecularMappingObject::Render(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResources(m_resourceHandles, 3, 0);
	m_model->Render(renderer, m_rpl);
}


PBRObject::PBRObject(
	const std::wstring& modelPath,
	const std::wstring& diffuseMapPath,
	const std::wstring& normalMapPath,
	const std::wstring& metallicMapPath,
	const std::wstring& roughnessMapPath,
	const std::wstring& aoMapPath,
	const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<RawTBNModel>(modelPath, 1, args);

	size_t flag = Texture2D::FLAG::DISCARD_SRGB;
	args[0] = &flag;

	m_diffuseMap = Resource::Get<Texture2D>(diffuseMapPath, 1, args);
	m_normalMap = Resource::Get<Texture2D>(normalMapPath, 1, args);
	m_metallicMap = Resource::Get<Texture2D>(metallicMapPath, 1, args);
	m_roughnessMap = Resource::Get<Texture2D>(roughnessMapPath, 1, args);

	if(!aoMapPath.empty())
		m_aoMap = Resource::Get<Texture2D>(aoMapPath, 1, args);
	else
#ifdef _DEBUG
		m_aoMap = Resource::Get<Texture2D>(L"D:/KEngine/ResourceFile/temp_img/white.png", 1, args);
#else
		m_aoMap = Resource::Get<Texture2D>(L"temp_img/white.png", 1, args);
#endif // _DEBUG


	m_resourceHandles[0] = m_diffuseMap->GetNativeHandle();
	m_resourceHandles[1] = m_normalMap->GetNativeHandle();
	m_resourceHandles[2] = m_metallicMap->GetNativeHandle();
	m_resourceHandles[3] = m_roughnessMap->GetNativeHandle();
	m_resourceHandles[4] = m_aoMap->GetNativeHandle();

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VS_TBN_Model",
		L"Model3D/PBR/PSPBRModel"
	);

}

PBRObject::~PBRObject()
{
	Resource::Release(&m_model);
	Resource::Release(&m_diffuseMap);
	Resource::Release(&m_normalMap);
	Resource::Release(&m_metallicMap);
	Resource::Release(&m_roughnessMap);
	Resource::Release(&m_aoMap);
}

void PBRObject::Render(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResources(m_resourceHandles, 5, 0);
	m_model->Render(renderer, m_rpl);
}

#define _STATIC_OBJECT_IMPL_GET_AABB(className)		\
AABB className::GetAABB()					\
{											\
	auto ret = GetLocalAABB();				\
	ret.Transform(m_transform);				\
	return ret;								\
}											\
AABB className::GetLocalAABB()				\
{											\
	AABB ret = {};							\
	Vec3 points[16];						\
	for (auto& mesh : m_model->Meshs())		\
	{										\
		if (ret.IsValid())					\
		{									\
			ret.GetPoints(points);			\
			mesh.aabb.GetPoints(&points[8]);\
			ret.FromPoints(points, 16);		\
		}									\
		else								\
		{									\
			ret = mesh.aabb;				\
		}									\
	}										\
	return ret;								\
}


PBRMultiMeshStaticObject::PBRMultiMeshStaticObject(const std::wstring& modelPath, 
	std::vector<PBRMaterialPath>& paths, const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<RawTBNModel>(modelPath, 1, args);

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VS_TBN_Model",
		L"Model3D/PBR/PSPBRModel"
	);

	m_meshsMaterial.resize(paths.size());
	size_t i = 0;
	for (auto& m : m_meshsMaterial)
	{
		m.From(paths[i]);
		i++;
		if (i == m_model->Meshs().size()) break;
	}
}

PBRMultiMeshStaticObject::PBRMultiMeshStaticObject(const std::wstring& modelPath, const Mat4x4& preTransform)
{
	std::wstring rootPath = PopPath(modelPath);
	std::vector<std::wstring> materials;
	void* args[] = { (void*)&preTransform, &materials };

	m_model = Resource::Get<RawTBNModel>(modelPath, 2, args);

	m_rpl = RenderPipelineManager::Get(
		R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
		}
		)",
		L"Model3D/VS_TBN_Model",
		L"Model3D/PBR/PSPBRModel"
	);

	for (auto& m : materials)
	{
		StandardPath(m);
	}

	m_meshsMaterial.resize(materials.size() / 5);

	PBRMaterialPath temp;

	size_t i = 0;
	for (auto& m : m_meshsMaterial)
	{
		auto id = i * 5;

		temp = { 
			materials[id	] == L"" ? L"" : CombinePath(rootPath, materials[id	   ]),
			materials[id + 1] == L"" ? L"" : CombinePath(rootPath, materials[id + 1]),
			materials[id + 2] == L"" ? L"" : CombinePath(rootPath, materials[id + 2]),
			materials[id + 3] == L"" ? L"" : CombinePath(rootPath, materials[id + 3]),
			materials[id + 4] == L"" ? L"" : CombinePath(rootPath, materials[id + 4])
		};

		m.From(temp);
		i++;
		if (i == m_model->Meshs().size()) break;
	}
}

PBRMultiMeshStaticObject::~PBRMultiMeshStaticObject()
{
	for (auto& m : m_meshsMaterial)
	{
		m.material.Free();
	}

	Resource::Release(&m_model);
}

void PBRMultiMeshStaticObject::Render(IRenderer* renderer)
{
	FlushTransform();

	size_t i = 0;
	auto loop = m_meshsMaterial.size();

	for (auto& mesh : m_model->m_meshs)
	{
		RenderPipeline::PSSetResources(m_meshsMaterial[i % loop].resourceHandles, 5, 0);
		RenderPipeline::VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		renderer->Render(m_rpl, mesh.vb, mesh.ib);
		i++;
	}
};

_STATIC_OBJECT_IMPL_GET_AABB(BasicObject);
_STATIC_OBJECT_IMPL_GET_AABB(NormalMappingObject);
_STATIC_OBJECT_IMPL_GET_AABB(NormalSpecularMappingObject);
_STATIC_OBJECT_IMPL_GET_AABB(PBRObject);
_STATIC_OBJECT_IMPL_GET_AABB(PBRMultiMeshStaticObject);
