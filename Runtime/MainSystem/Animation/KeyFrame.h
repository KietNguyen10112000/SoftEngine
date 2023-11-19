#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

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

NAMESPACE_END