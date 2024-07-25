#include "Animation.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

KeyFrames* Animation::GetRootMotionKeyFrames(AnimModel* model)
{
	auto rootBoneId = model->m_rootBoneNodeId;

	auto idx = m_nodeToChannelId[rootBoneId];
	if (idx != INVALID_ID)
	{
		return &m_motion->m_channels[idx];
	}

	return nullptr;
}

KeyFrames* Animation::GetKeyFrames(ID nodeId)
{
	auto idx = m_nodeToChannelId[nodeId];
	if (idx != INVALID_ID)
	{
		return &m_motion->m_channels[idx];
	}

	return nullptr;
}

std::vector<ActionInterpolation<Transform>::KeyFrame> Animation::ConvertToActionKeyFrames(KeyFrames* keyframes, float tickPerSecond)
{
	using AKeyFrame = ActionInterpolation<Transform>::KeyFrame;

	std::vector<ActionInterpolation<Transform>::KeyFrame> ret;
	
	auto& scaling = keyframes->scaling;
	auto& rotation = keyframes->rotation;
	auto& translation = keyframes->translation;

	KeyFramesIndex index = {};
	while (index.s != scaling.size() || index.r != rotation.size() || index.t != translation.size())
	{
		AKeyFrame keyframe = { {}, 0 };

		auto sTime = index.s == scaling.size() ? INFINITY : scaling[index.s].time;
		auto rTime = index.r == rotation.size() ? INFINITY : rotation[index.r].time;
		auto tTime = index.t == translation.size() ? INFINITY : translation[index.t].time;

		auto min = std::min(std::min(sTime, rTime), tTime);

		keyframe.time = min / tickPerSecond;

		if (min == sTime)
		{
			keyframe.value.Scale() = scaling[index.s].value;
			index.s++;
		}

		if (min == rTime)
		{
			keyframe.value.Rotation() = rotation[index.r].value;
			index.r++;
		}

		if (min == tTime)
		{
			keyframe.value.Position() = translation[index.t].value;
			index.t++;
		}

		ret.push_back(keyframe);
	}

	return ret;
}

NAMESPACE_END