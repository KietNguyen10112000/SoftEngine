#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class Animator
{
public:
	struct AnimationData
	{
		float duration;
	};

	ID m_animationId;
	float m_duration;

};

NAMESPACE_END