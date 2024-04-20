#pragma once

#include "KeyFrame.h"
#include "Resources/AnimMotion.h"

NAMESPACE_BEGIN

class Animation
{
private:
	friend class AnimMotion;
	friend class AnimModel;

	Resource<AnimMotion> m_motion;
	std::vector<ID> m_channelToNodeId;
	std::vector<AABoxKeyFrames> m_animMeshLocalAABoxKeyFrames;

	inline Animation() {};

public:
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

	inline AABoxKeyFrames& GetAABBKeyFrames(size_t boneId)
	{
		return m_animMeshLocalAABoxKeyFrames[boneId];
	}

	inline float GetTicksPerSecond() const
	{
		return m_motion->m_ticksPerSecond;
	}

	inline float GetTickDuration() const
	{
		return m_motion->m_tickDuration;
	}

	inline void InitializeTrack(AnimationTrack* track, float startTime, float endTime)
	{
		auto startTick = startTime < 0 ? 0 : startTime * GetTicksPerSecond();
		auto endTick = endTime < 0 ? GetTickDuration() : endTime * GetTicksPerSecond();

		auto& startIndex = track->startKeyFramesIndex;
		auto& startAABBIndex = track->startAABBKeyFrameIndex;

		auto num = (uint32_t)m_boneToChannelId.size();
		startIndex.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto pChannel = GetKeyFrames(i);
			if (!pChannel)
			{
				continue;
			}

			auto& channel = *pChannel;
			auto& index = startIndex[i];

			channel.BinaryFindScale(startTick, &index.s);
			channel.BinaryFindRotation(startTick, &index.r);
			channel.BinaryFindTranslation(startTick, &index.t);
		}

		num = (uint32_t)m_animMeshLocalAABoxKeyFrames.size();
		startAABBIndex.resize(num);
		for (uint32_t i = 0; i < num; i++)
		{
			auto& channel = m_animMeshLocalAABoxKeyFrames[i];
			auto& index = startAABBIndex[i];

			channel.BinaryFind(startTick, &index);
		}

		track->startTick = startTick;
		track->tickDuration = endTick - startTick;
		track->ticksPerSecond = GetTicksPerSecond();
	}

};

NAMESPACE_END