#pragma once

#include "AnimationComponent.h"

//#include "Core/Memory/Structures/Raw/SmartPointers.h"

#include "Resources/AnimModel.h"

#include "AnimatorSkeletalGameObject.h"

NAMESPACE_BEGIN

class AnimatorSkeletalGameObject;

class AnimSkeletalGameObject : public AnimationComponent
{
public:
	Resource<AnimModel>	m_model3D;
	Handle<AnimatorSkeletalGameObject> m_animator;
	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	// refer to m_animMeshRenderingBuffer.bones[m_boneId], m_model3D.m_animations.channels[m_boneId]
	ID m_boneId;

	ID m_animationSystemId = INVALID_ID;

	ID m_numAnimIterationCount = 0;

	KeyFramesIndex m_keyFramesIndex;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_animator);
	}

public:
	COMPONENT_CLASS(AnimSkeletalGameObject);

	AnimSkeletalGameObject();

	// Inherited via AnimationComponent
	virtual void OnComponentAdded() override;
	virtual void OnComponentRemoved() override;
	virtual void OnTransformChanged() override;
	virtual AABox GetGlobalAABB() override;
	virtual void Serialize(soft::Serializer* serializer);
	virtual void Deserialize(soft::Serializer* serializer);
	virtual void CleanUp();
	virtual soft::Handle<soft::ClassMetadata> GetMetadata(size_t sign);
	virtual void OnPropertyChanged(const soft::UnknownAddress& var, const soft::Variant& newValue);

public:
	void Update(float dt);

	inline const auto& GetKeyFrames() const
	{
		auto& keyFrames = m_model3D->m_animations[m_animator->GetCurrentAnimationId()].channels[m_boneId];
		return keyFrames;
	}

};

NAMESPACE_END