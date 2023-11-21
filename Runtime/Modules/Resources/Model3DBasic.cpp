#include "Model3DBasic.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils 
{
	extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);
}

Model3DBasic::Model3DBasic(String path, bool placeholder) : ResourceBase(path)
{
	if (placeholder)
	{
		return;
	}

	auto fs = FileSystem::Get();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesRelativePath(path).c_str(), 
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);
	ResourceUtils::LoadAllMeshsForModel3DBasic(this, scene, true);
}

NAMESPACE_END