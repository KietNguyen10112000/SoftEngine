#pragma once

#include "Core/TypeDef.h"

#include "Resources/AnimModel.h"

#include "Animator.h"

NAMESPACE_BEGIN

class AnimatorSkeletalGameObject : public Animator
{
public:
	using Base = Animator;

	ID m_animationId = 0;

	Resource<AnimModel>	m_model3D;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	Array<Handle<GameObject>> m_animMeshRendererObjs;

	// duration in sec
	float m_tickDuration;
	float m_ticksPerSecond;

	float m_t = 0;

	uint32_t m_aabbKeyFrameIndex = 0;

	size_t m_numUpdateAABB = 0;

	size_t m_numAnimIterationCount = 0;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
		tracer->Trace(m_animMeshRendererObjs);
	}

public:
	COMPONENT_CLASS(AnimatorSkeletalGameObject);

	AnimatorSkeletalGameObject();

private:
	inline void SetAnimationImpl(ID animationId, float blendTime)
	{
		m_animationId = animationId;
		m_t = 0;
		m_numAnimIterationCount++;

		auto& animation = m_model3D->m_animations[m_animationId];
		m_tickDuration = animation.tickDuration;
		m_ticksPerSecond = animation.ticksPerSecond;
	}

public:

	inline virtual ID GetCurrentAnimationId() const override
	{
		return m_animationId;
	}

	inline void Update(float dt)
	{
		m_t += dt * m_ticksPerSecond;
		if (m_t > m_tickDuration)
		{
			uint32_t num = (uint32_t)std::floor(m_t / m_tickDuration);
			m_t -= num * m_tickDuration;

			m_numAnimIterationCount += num;
		}
	}


	// Inherited via Animator
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

	virtual ID FindAnimation(const String& name) override;

	virtual void GetAnimationsName(std::vector<String>& output) const override;

	virtual void SetDuration(float sec) override;

	virtual void SetDuration(float sec, ID animationId) override;

	virtual float GetDuration() const override;

	virtual void Play(ID animationId, float blendTime) override;

	virtual void Serialize(soft::Serializer* serializer);

	virtual void Deserialize(soft::Serializer* serializer);

	virtual void CleanUp();

	virtual soft::Handle<soft::ClassMetadata> GetMetadata(size_t sign);

	virtual void OnPropertyChanged(const soft::UnknownAddress& var, const soft::Variant& newValue);

};

NAMESPACE_END