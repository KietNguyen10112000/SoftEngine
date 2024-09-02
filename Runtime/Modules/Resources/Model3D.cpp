#include "Model3D.h"

#include "Texture2D.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene/GameObject.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

NAMESPACE_BEGIN

namespace ResourceUtils 
{
	extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);
}

int Model3D::Load(const String& path)
{
	auto fs = FileSystem::Get();

	//String defaultDiffusePath = Texture2D::DEFAULT_FILE;

	std::vector<String> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	if (scene == nullptr)
	{
		return -1;
	}

	constexpr static void (*ProcessNode)(Model3D*, const aiScene*, aiNode*, ID) =
		[](Model3D* model, const aiScene* scene, aiNode* node, ID parentId) {

		aiVector3D scale;
		aiQuaternion rot;
		aiVector3D pos;
		node->mTransformation.Decompose(scale, rot, pos);

		Transform transform = {};
		transform.Scale() = reinterpret_cast<const Vec3&>(scale);
		transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
		transform.Position() = reinterpret_cast<const Vec3&>(pos);

		auto& nodes = model->m_nodes;
		
		auto myId = nodes.size();

		String name;
		if (node->mName.length != 0)
		{
			name = node->mName.C_Str();
		}

		ID meshId = INVALID_ID;
		if (node->mNumMeshes > 1)
		{
			nodes.push_back({ name,parentId,INVALID_ID,transform });
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				nodes.push_back({ String(),myId,(ID)node->mMeshes[i],{} });
			}
		}
		else if (node->mNumMeshes > 0)
		{
			meshId = node->mMeshes[0];
			nodes.push_back({ name,parentId,meshId,transform });
		}
		else
		{
			nodes.push_back({ name,parentId,INVALID_ID,transform });
		}

		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(model, scene, node->mChildren[i], myId);
		}
	};

	if (scene->HasMaterials())
	{
		auto* materials = scene->mMaterials;
		auto materialsCount = scene->mNumMaterials;

		for (size_t i = 0; i < materialsCount; i++)
		{
			auto material = materials[i];

			aiString file;
			//material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), file);
			material->GetTexture(aiTextureType_DIFFUSE, 0, &file);

			std::string str = file.C_Str();
			std::replace(str.begin(), str.end(), '\\', '/');
			String diffusePath = basePath + str.c_str();// fs->GetResourcesRelativePath(basePath + str.c_str());

			//assert(0 && "TODO: Check this again!!!");

			if (!str.empty() && fs->IsFileExisted(diffusePath.c_str()))
			{
				diffuseTextures.push_back(FileSystem::Get()->GetRelativeFilePath(diffusePath));
			}
			else
			{
				diffuseTextures.push_back({});
			}
		}
	}

	diffuseTextures.push_back({});

	ResourceUtils::LoadAllMeshsForModel3DBasic(this, scene, true);

	{
		uint32_t count = 0;
		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			auto& mesh = scene->mMeshes[i];
			if (mesh->HasBones())
			{
				continue;
			}

			if (mesh->mMaterialIndex >= 0)
			{
				m_meshes[i].m_defaultDiffusePath = diffuseTextures[mesh->mMaterialIndex];
			}
			else
			{
				m_meshes[i].m_defaultDiffusePath = "";
			}

			count++;
		}
	}

	ProcessNode(this, scene, scene->mRootNode, INVALID_ID);

	return 0;
}

Handle<GameObject> Model3D::MakeGameObject()
{
	auto model3D = resource::StaticCast<Model3D>(GetSelfResource());

	auto ret = mheap::New<GameObject>();

	std::vector<GameObject*> objs;
	objs.reserve(m_nodes.size());
	objs.push_back(ret);

	auto& nodes = m_nodes;

	ret->SetLocalTransform(nodes[0].localTransform);
	ret->Name() = nodes[0].name;

	auto count = nodes.size();
	for (size_t i = 1; i < count; i++)
	{
		auto& node = nodes[i];
		auto& parent = objs[node.parentId];

		auto obj = mheap::New<GameObject>();
		obj->SetLocalTransform(node.localTransform);
		obj->Name() = node.name;


		if (node.meshId != INVALID_ID)
		{
			auto comp = obj->NewComponent<MeshBasicRenderer>(false);
			comp->m_mesh = &m_meshes[node.meshId];
			comp->m_model3D = model3D;

			auto& path = comp->m_mesh->m_defaultDiffusePath;

			if (path.empty())
			{
				comp->m_texture = resource::Load<Texture2D>(Texture2D::DEFAULT_FILE);
			}
			else
			{
				comp->m_texture = resource::Load<Texture2D>(path);
			}
		}

		parent->AddChild(obj);
		objs.push_back(obj);
	}

	return ret;
}

NAMESPACE_END