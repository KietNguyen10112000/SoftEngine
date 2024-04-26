#include "Utils.h"

#include "FileSystem/FileSystem.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../AnimMotion.h"

NAMESPACE_BEGIN

namespace ResourceUtils
{

void LoadAnimMotion(String filePath, void* _aiScene, std::vector<Resource<AnimMotion>>& output)
{
	auto scene = (const aiScene*)_aiScene;

	constexpr static auto ExtractScaling = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumScalingKeys;
		auto aiScalings = aiNode->mScalingKeys;

		keyFrames.scaling.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiScaling = aiScalings[i];
			auto& scaling = keyFrames.scaling[i];

			scaling.time = aiScaling.mTime;
			scaling.value.x = aiScaling.mValue.x;
			scaling.value.y = aiScaling.mValue.y;
			scaling.value.z = aiScaling.mValue.z;
		}
	};

	constexpr static auto ExtractRotation = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumRotationKeys;
		auto aiKeys = aiNode->mRotationKeys;

		keyFrames.rotation.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiKey = aiKeys[i];
			auto& key = keyFrames.rotation[i];

			key.time = aiKey.mTime;
			key.value.x = aiKey.mValue.x;
			key.value.y = aiKey.mValue.y;
			key.value.z = aiKey.mValue.z;
			key.value.w = aiKey.mValue.w;
		}
	};

	constexpr static auto ExtractTranslation = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumPositionKeys;
		auto aiKeys = aiNode->mPositionKeys;

		keyFrames.translation.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiKey = aiKeys[i];
			auto& key = keyFrames.translation[i];

			key.time = aiKey.mTime;
			key.value.x = aiKey.mValue.x;
			key.value.y = aiKey.mValue.y;
			key.value.z = aiKey.mValue.z;
		}
	};

	auto numAnimations = scene->mNumAnimations;
	for (uint32_t i = 0; i < numAnimations; i++)
	{
		auto aiAnim = scene->mAnimations[i];
		
		auto animMotion = resource::Load<AnimMotion>(String::Format("{}|{}", filePath, i), true);

		animMotion->m_name = aiAnim->mName.C_Str();
		animMotion->m_tickDuration = aiAnim->mDuration;
		animMotion->m_ticksPerSecond = aiAnim->mTicksPerSecond;

		auto numChannels = aiAnim->mNumChannels;

		animMotion->m_channels.resize(numChannels);

		for (uint32_t j = 0; j < numChannels; j++)
		{
			auto aiAnimNode = aiAnim->mChannels[j];
			String affectedNodeName = aiAnimNode->mNodeName.C_Str();

			animMotion->m_nodeNameEffectedByChannel.push_back(affectedNodeName);

			auto& channel = animMotion->m_channels[j];
			
			ExtractScaling(aiAnimNode, channel);
			ExtractRotation(aiAnimNode, channel);
			ExtractTranslation(aiAnimNode, channel);
		}

		output.push_back(animMotion);
	}
}

std::vector<Resource<AnimMotion>> LoadAnimMotion(String path)
{
	auto fs = FileSystem::Get();

	std::vector<Resource<AnimMotion>> motions;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	LoadAnimMotion(path, (void*)scene, motions);

	return motions;
}

}

NAMESPACE_END