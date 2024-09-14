#pragma once

#include "KeyFrame.h"
#include "Resources/AnimMotion.h"

#include "Common/Actions/ActionInterpolation.h"

NAMESPACE_BEGIN

class API Animation
{
public:
	struct ActionInterpolationKeyFrames
	{
		std::vector<ActionInterpolation<Vec3>::KeyFrame> scaling;
		std::vector<ActionInterpolation<Quaternion>::KeyFrame> rotation;
		std::vector<ActionInterpolation<Vec3>::KeyFrame> translation;
	};

private:
	friend class AnimMotion;
	friend class AnimModel;
	friend class AnimationSystem;
	friend class AnimatorSkeletalGameObject;

	Resource<AnimMotion> m_motion;
	std::vector<ID> m_nodeToChannelId;
	//std::vector<AABoxKeyFrames> m_animMeshLocalAABoxKeyFrames;

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

	/*inline auto& GetMeshLocalAABBKeyFrames()
	{
		return m_animMeshLocalAABoxKeyFrames;
	}*/

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

	inline float GetDuration() const
	{
		return m_motion->m_tickDuration / m_motion->m_ticksPerSecond;
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

	inline static std::vector<ActionInterpolation<Vec3>::KeyFrame> ExtractScaling(const std::vector<ActionInterpolation<Transform>::KeyFrame>& keyframes)
	{
		std::vector<ActionInterpolation<Vec3>::KeyFrame> ret;
		for (auto& k : keyframes)
		{
			ret.push_back({ k.value.GetScale(),k.time });
		}
		return ret;
	}

	inline static std::vector<ActionInterpolation<Quaternion>::KeyFrame> ExtractRotation(const std::vector<ActionInterpolation<Transform>::KeyFrame>& keyframes)
	{
		std::vector<ActionInterpolation<Quaternion>::KeyFrame> ret;
		for (auto& k : keyframes)
		{
			ret.push_back({ k.value.GetRotation(),k.time });
		}
		return ret;
	}

	inline static std::vector<ActionInterpolation<Vec3>::KeyFrame> ExtractTranslation(const std::vector<ActionInterpolation<Transform>::KeyFrame>& keyframes)
	{
		std::vector<ActionInterpolation<Vec3>::KeyFrame> ret;
		for (auto& k : keyframes)
		{
			ret.push_back({ k.value.GetPosition(),k.time });
		}
		return ret;
	}

	static ActionInterpolationKeyFrames ConvertToActionKeyFramesPerChannel(KeyFrames* keyframes, float tickPerSecond);

};

NAMESPACE_END