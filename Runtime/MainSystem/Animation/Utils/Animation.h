#pragma once

#include "KeyFrame.h"
#include "Resources/AnimMotion.h"

#include "Common/Actions/ActionInterpolation.h"

NAMESPACE_BEGIN

class Animation
{
private:
	friend class AnimMotion;
	friend class AnimModel;
	friend class AnimationSystem;
	friend class AnimatorSkeletalGameObject;

	Resource<AnimMotion> m_motion;
	std::vector<ID> m_nodeToChannelId;
	std::vector<AABoxKeyFrames> m_animMeshLocalAABoxKeyFrames;

public:
	inline Animation() {};

	/*inline KeyFrames* GetKeyFrames(size_t boneId)
	{
		auto id = m_boneToChannelId[boneId];
		if (id == INVALID_ID)
		{
			return nullptr;
		}

		return &m_motion->m_channels[id];
	}*/
	inline auto& GetChannels()
	{
		return m_motion->m_channels;
	}

	inline auto& GetMeshLocalAABBKeyFrames()
	{
		return m_animMeshLocalAABoxKeyFrames;
	}

	inline auto& GetNodeToChannelId()
	{
		return m_nodeToChannelId;
	}

	inline float GetTicksPerSecond() const
	{
		return m_motion->m_ticksPerSecond;
	}

	inline float GetTickDuration() const
	{
		return m_motion->m_tickDuration;
	}

	inline auto Name() const
	{
		return m_motion->m_name;
	}

	inline auto& GetMotion() const
	{
		return m_motion;
	}

	KeyFrames* GetRootMotionKeyFrames(AnimModel* model);
	KeyFrames* GetKeyFrames(ID nodeId);

	static std::vector<ActionInterpolation<Transform>::KeyFrame> ConvertToActionKeyFrames(KeyFrames* keyframes, float tickPerSecond);

};

NAMESPACE_END