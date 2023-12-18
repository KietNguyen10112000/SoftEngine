#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "Core/Structures/String.h"
#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

template<class T>
struct KeyFrame
{
	T value = {};
	float time = 0;
};

typedef KeyFrame<Vec3> ScalingKeyFrame;
typedef KeyFrame<Quaternion> RotaionKeyFrame;
typedef KeyFrame<Vec3> TranslationKeyFrame;

typedef KeyFrame<AABox> AABoxKeyFrame;

struct KeyFramesIndex
{
	uint32_t s = 0;
	uint32_t r = 0;
	uint32_t t = 0;
};

struct KeyFrames
{
	std::vector<ScalingKeyFrame> scaling;
	std::vector<RotaionKeyFrame> rotation;
	std::vector<TranslationKeyFrame> translation;

	inline Vec3 FindScale(uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		auto& keyFrames = scaling;
		uint32_t num = (uint32_t)keyFrames.size();

		assert(startIndex < num);

		for (uint32_t i = startIndex; i < num; i++)
		{
			auto& keyFrame = keyFrames[i];
			if (keyFrame.time > t)
			{
				if (outputIndex)
				{
					*outputIndex = i;
				}

				if (i == 0)
				{
					return keyFrame.value;
				}

				auto l0 = t - keyFrames[i - 1].time;
				auto l1 = keyFrames[i].time - keyFrames[i - 1].time;
				return Lerp(
						keyFrames[i - 1].value,
						keyFrames[i].value,
						l0 / l1
					);
			}
		}

		if (outputIndex)
		{
			*outputIndex = num - 1;
		}
		return keyFrames.back().value;
	}

	inline void FindScaleMatrix(Mat4* out, uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		out->SetScale(FindScale(outputIndex, startIndex, t));
	}

	inline Quaternion FindRotation(uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		auto& keyFrames = rotation;
		auto num = (uint32_t)keyFrames.size();

		assert(startIndex < num);

		for (uint32_t i = startIndex; i < num; i++)
		{
			auto& keyFrame = keyFrames[i];
			if (keyFrame.time > t)
			{
				if (outputIndex)
				{
					*outputIndex = i;
				}

				if (i == 0)
				{
					return keyFrame.value;
				}

				auto l0 = t - keyFrames[i - 1].time;
				auto l1 = keyFrames[i].time - keyFrames[i - 1].time;
				return SLerp(
						keyFrames[i - 1].value,
						keyFrames[i].value,
						l0 / l1
					);
			}
		}

		if (outputIndex)
		{
			*outputIndex = num - 1;
		}
		return keyFrames.back().value;
	}

	inline void FindRotationMatrix(Mat4* out, uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		out->SetRotation(FindRotation(outputIndex, startIndex, t));
	}

	inline Vec3 FindTranslation(uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		auto& keyFrames = translation;
		auto num = (uint32_t)keyFrames.size();

		assert(startIndex < num);

		for (uint32_t i = startIndex; i < num; i++)
		{
			auto& keyFrame = keyFrames[i];
			if (keyFrame.time > t)
			{
				if (outputIndex)
				{
					*outputIndex = i;
				}

				if (i == 0)
				{
					return keyFrame.value;
				}

				auto l0 = t - keyFrames[i - 1].time;
				auto l1 = keyFrames[i].time - keyFrames[i - 1].time;
				return Lerp(
						keyFrames[i - 1].value,
						keyFrames[i].value,
						l0 / l1
					);
			}
		}

		if (outputIndex)
		{
			*outputIndex = num - 1;
		}
		return keyFrames.back().value;
	}

	inline void FindTranslationMatrix(Mat4* out, uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		out->SetTranslation(FindTranslation(outputIndex, startIndex, t));
	}


	inline Vec3 BinaryFindScale(float t, uint32_t* outputIndex) const
	{
		auto& keyFrames = scaling;
		uint32_t num = (uint32_t)keyFrames.size();
		uint32_t count = num;
		uint32_t idx = 0;

		while (count)
		{
			auto step = count / 2;
			auto it = idx + step;

			auto& v = keyFrames[it];

			if (v.time < t)
			{
				idx = it + 1;
				count -= step + 1;
			}
			else
			{
				count = step;
			}
		}

		auto& v = keyFrames[idx];

		if (outputIndex)
		{
			*outputIndex = idx;
		}

		if (v.time < t)
		{
			assert(idx == num - 1);

			/*if (idx != num - 1)
			{
				BinaryFindScale(t);
			}*/

			return keyFrames.back().value;
		}

		if (idx == 0)
		{
			return keyFrames.front().value;
		}

		auto l0 = t - keyFrames[idx - 1].time;
		auto l1 = keyFrames[idx].time - keyFrames[idx - 1].time;
		return Lerp(
			keyFrames[idx - 1].value,
			keyFrames[idx].value,
			l0 / l1
		);
	}

	inline Quaternion BinaryFindRotation(float t, uint32_t* outputIndex) const
	{
		auto& keyFrames = rotation;
		uint32_t num = (uint32_t)keyFrames.size();
		uint32_t count = num;
		uint32_t idx = 0;

		while (count)
		{
			auto step = count / 2;
			auto it = idx + step;

			auto& v = keyFrames[it];

			if (v.time < t)
			{
				idx = it + 1;
				count -= step + 1;
			}
			else
			{
				count = step;
			}
		}

		auto& v = keyFrames[idx];

		if (outputIndex)
		{
			*outputIndex = idx;
		}

		if (v.time < t)
		{
			assert(idx == num - 1);
			return keyFrames.back().value;
		}

		if (idx == 0)
		{
			return keyFrames.front().value;
		}

		auto l0 = t - keyFrames[idx - 1].time;
		auto l1 = keyFrames[idx].time - keyFrames[idx - 1].time;
		return SLerp(
			keyFrames[idx - 1].value,
			keyFrames[idx].value,
			l0 / l1
		);
	}

	inline Vec3 BinaryFindTranslation(float t, uint32_t* outputIndex) const
	{
		auto& keyFrames = translation;
		uint32_t num = (uint32_t)keyFrames.size();
		uint32_t count = num;
		uint32_t idx = 0;

		while (count)
		{
			auto step = count / 2;
			auto it = idx + step;

			auto& v = keyFrames[it];

			if (v.time < t)
			{
				idx = it + 1;
				count -= step + 1;
			}
			else
			{
				count = step;
			}
		}

		auto& v = keyFrames[idx];

		if (outputIndex)
		{
			*outputIndex = idx;
		}

		if (v.time < t)
		{
			assert(idx == num - 1);
			return keyFrames.back().value;
		}

		if (idx == 0)
		{
			return keyFrames.front().value;
		}

		auto l0 = t - keyFrames[idx - 1].time;
		auto l1 = keyFrames[idx].time - keyFrames[idx - 1].time;
		return Lerp(
			keyFrames[idx - 1].value,
			keyFrames[idx].value,
			l0 / l1
		);
	}
};

struct AABoxKeyFrames
{
	std::vector<AABoxKeyFrame> aaBox;

	AABox boundAABox;

	inline AABox Find(uint32_t* outputIndex, uint32_t startIndex, float t) const
	{
		auto& keyFrames = aaBox;
		auto num = (uint32_t)keyFrames.size();

		assert(startIndex < num);

		for (uint32_t i = startIndex; i < num; i++)
		{
			auto& keyFrame = keyFrames[i];
			if (keyFrame.time > t)
			{
				if (outputIndex)
				{
					*outputIndex = i;
				}

				if (i == 0)
				{
					return keyFrame.value;
				}

				auto& start = keyFrames[i - 1];
				auto& end = keyFrames[i];
				auto l0 = t - start.time;
				auto l1 = end.time - start.time;
				auto d = l0 / l1;

				AABox ret;
				ret.m_center = Lerp(start.value.m_center, end.value.m_center, d);
				ret.m_halfDimensions = Lerp(start.value.m_halfDimensions, end.value.m_halfDimensions, d);
				return ret;
			}
		}

		if (outputIndex)
		{
			*outputIndex = num - 1;
		}
		return keyFrames.back().value;
	}

	inline AABox BinaryFind(float t, uint32_t* outputIndex) const
	{
		auto& keyFrames = aaBox;
		uint32_t num = (uint32_t)keyFrames.size();
		uint32_t count = num;
		uint32_t idx = 0;

		while (count)
		{
			auto step = count / 2;
			auto it = idx + step;

			auto& v = keyFrames[it];

			if (v.time < t)
			{
				idx = it + 1;
				count -= step + 1;
			}
			else
			{
				count = step;
			}
		}

		auto& v = keyFrames[idx];

		if (outputIndex)
		{
			*outputIndex = idx;
		}

		if (v.time < t)
		{
			assert(idx == num - 1);
			return keyFrames.back().value;
		}

		if (idx == 0)
		{
			return keyFrames.front().value;
		}

		auto& start = keyFrames[idx - 1];
		auto& end = keyFrames[idx];
		auto l0 = t - start.time;
		auto l1 = end.time - start.time;
		auto d = l0 / l1;

		AABox ret;
		ret.m_center = Lerp(start.value.m_center, end.value.m_center, d);
		ret.m_halfDimensions = Lerp(start.value.m_halfDimensions, end.value.m_halfDimensions, d);
		return ret;
	}
};

struct AnimationTrack
{
	ID animationId = INVALID_ID;

	std::vector<KeyFramesIndex> startKeyFramesIndex;

	std::vector<uint32_t> startAABBKeyFrameIndex;

	float tickDuration;
	float ticksPerSecond;
	float startTick;
};

struct Animation
{
	String name;
	float tickDuration = 0;
	float ticksPerSecond = 0;

	std::vector<KeyFrames> channels;

	// keyframes of each anim mesh's AABox when animation is applied to,
	// it will be refered by AnimMesh::m_model3DIdx
	std::vector<AABoxKeyFrames> animMeshLocalAABoxKeyFrames;

	// time in sec
	inline void InitializeTrack(AnimationTrack* track, float startTime, float endTime)
	{
		auto startTick = startTime < 0 ? 0 : startTime * ticksPerSecond;
		auto endTick = endTime < 0 ? tickDuration : endTime * ticksPerSecond;

		auto& startIndex = track->startKeyFramesIndex;
		auto& startAABBIndex = track->startAABBKeyFrameIndex;

		auto num = (uint32_t)channels.size();
		startIndex.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& channel = channels[i];
			auto& index = startIndex[i];

			channel.BinaryFindScale(startTick, &index.s);
			channel.BinaryFindRotation(startTick, &index.r);
			channel.BinaryFindTranslation(startTick, &index.t);
		}

		num = (uint32_t)animMeshLocalAABoxKeyFrames.size();
		startAABBIndex.resize(num);
		for (uint32_t i = 0; i < num; i++)
		{
			auto& channel = animMeshLocalAABoxKeyFrames[i];
			auto& index = startAABBIndex[i];

			channel.BinaryFind(startTick, &index);
		}

		track->startTick = startTick;
		track->tickDuration = endTick - startTick;
		track->ticksPerSecond = ticksPerSecond;
	}
};

NAMESPACE_END