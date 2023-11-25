#pragma once

#include "AnimationComponent.h"

//#include "Core/Memory/Structures/Raw/SmartPointers.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class Animator;

class AnimSkeletalGameObject : public AnimationComponent
{
public:
	Resource<AnimModel>	m_model3D;
	SharedPtr<Animator> m_animator;
	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	// refer to m_animMeshRenderingBuffer.bones[m_boneId], m_model3D.m_animations.channels[m_boneId]
	ID m_boneId;

	ID m_animationSystemId = INVALID_ID;
};

NAMESPACE_END