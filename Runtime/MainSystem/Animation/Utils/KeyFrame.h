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


	inline Vec3 BinaryFindScale(float t) const
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

	inline Quaternion BinaryFindRotation(float t) const
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

	inline Vec3 BinaryFindTranslation(float t) const
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
};

NAMESPACE_END