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

struct KeyFrames
{
	std::vector<ScalingKeyFrame> scaling;
	std::vector<RotaionKeyFrame> rotation;
	std::vector<TranslationKeyFrame> translation;
};

struct Animation
{
	String name;
	float tickDuration = 0;
	float ticksPerSecond = 0;

	std::vector<KeyFrames> channels;
};

NAMESPACE_END