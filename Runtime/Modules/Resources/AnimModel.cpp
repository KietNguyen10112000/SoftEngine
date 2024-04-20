#include "AnimModel.h"

NAMESPACE_BEGIN

const char* AnimModel::CACHE_EXTENSION = ".AnimModel";

AnimModel::AnimModel(String path, bool placeholder) : Model3DBasic(path, true)
{
}

ID AnimModel::AddAnimation(const Resource<AnimMotion>& motion)
{
	auto animationId = m_animations.size();
	auto animation = std::make_unique<Animation>();

	animation->m_motion = motion;

	auto numBone = m_boneOffsetMatrixs.size();
	animation->m_boneToChannelId.resize(numBone);

	// map channel to bone
	for (size_t i = 0; i < numBone; i++)
	{
		auto& boneName = m_boneNames[i];
		auto it = motion->m_nodeNameEffectedByChannel.find(boneName);

		if (it == motion->m_nodeNameEffectedByChannel.end())
		{
			animation->m_boneToChannelId[i] = INVALID_ID;
		}

		animation->m_boneToChannelId[i] = it->second;
	}

	// load AABB key frames for this animation

	auto myPath = GetPath();
	auto modelPath = motion->GetModelFilePath();
	ByteStream stream;
	modelPath = "Resources/" + modelPath;
	auto streamPath = (myPath + "." + modelPath + "." + String::From(animationId) + "." + AnimModel::CACHE_EXTENSION);
	if (FileSystem::Get()->IsFileChanged(myPath.c_str())
		|| FileSystem::Get()->IsFileChanged(modelPath.c_str()) 
		|| !FileSystem::Get()->ReadStream(streamPath.c_str(), &stream))
	{

	}
	else
	{

	}

	m_animations.push_back(animation);
	return animationId;
}

NAMESPACE_END