#include "Utils.h"

#include "../AnimModel.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils
{
extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);

void LoadAllAnimMeshsForAnimModel(AnimModel* model, const aiScene* scene)
{
	model->m_meshes.resize(scene->mNumMeshes);
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto aiMesh = scene->mMeshes[i];
		auto mesh = &model->m_meshes[i];

		//LoadMesh(mesh, scene, aiMesh);

		mesh->m_model3DIdx = i;
	}
}

Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath)
{
	auto fs = FileSystem::Get();

	if (defaultDiffusePath.empty())
	{
		defaultDiffusePath = Texture2D::DEFAULT_FILE;
	}

	auto ret = mheap::New<GameObject>();
	auto model3D = resource::Load<AnimModel>(path, true);

	std::vector<Resource<Texture2D>> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

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
			String diffusePath = fs->GetResourcesRelativePath(basePath + str.c_str());

			if (fs->IsResourceExist(diffusePath.c_str()))
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(diffusePath.c_str()));
			}
			else
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(defaultDiffusePath));
			}
		}
	}

	diffuseTextures.push_back(resource::Load<Texture2D>(defaultDiffusePath));

	LoadAllMeshsForModel3DBasic(model3D, scene, false);

	return ret;
}

}

NAMESPACE_END