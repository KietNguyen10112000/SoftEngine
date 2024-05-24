#include "AnimMotion.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils
{
	extern void LoadAnimMotion(String filePath, void* _aiScene, std::vector<Resource<AnimMotion>>& output);
	extern void ExtractAnimMotionData(void* _aiNode, AnimMotion* animMotion);
}

//AnimMotion::AnimMotion(String path, bool placeHolder) : ResourceBase(path)
//{
//	if (placeHolder)
//	{
//		return;
//	}
//
//	LoadFromFile(path);
//}

int AnimMotion::Load(const String& path)
{
	return LoadFromFile(path);
}

int AnimMotion::LoadFromFile(const String& path)
{
	auto modelFilePath = GetModelFilePath();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelFilePath.c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	if (scene == nullptr)
	{
		return -1;
	}

	{
		auto rcPath = GetPath();

		std::string_view str = rcPath.c_str();
		auto idx = str.find_last_of('|');

		auto i = std::stoi(str.data() + idx + 1);

		ResourceUtils::ExtractAnimMotionData(scene->mAnimations[i], this);
	}

	return 0;
}

String AnimMotion::GetModelFilePath() const
{
	auto ret = GetPath();

	std::string_view str = ret.c_str();
	auto idx = str.find_last_of('|');

	assert(idx != std::string_view::npos);

	return String(ret.c_str(), idx);
}

NAMESPACE_END