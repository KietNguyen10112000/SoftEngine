#pragma once

#include "Animator.h"
#include "ANIMATION_TYPE.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class AnimatorSkeletalArray : public Animator
{
public:
	/*struct BoundNodeId
	{
		ID nodeId;
		Transform localTransform;
	};*/

	ID m_animationSystemId = 0;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	// include both animMesh and static mesh
	Array<Handle<GameObject>> m_meshRendererObjs;

	std::vector<Mat4> m_globalTransforms;

	std::vector<KeyFramesIndex> m_keyFramesIndex;

	std::vector<uint32_t> m_aabbKeyFrameIndex;

	ID m_animationId = 0;
	float m_tickDuration;
	float m_ticksPerSecond;
	float m_t = 0;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_meshRendererObjs);
	}

public:
	COMPONENT_CLASS(AnimatorSkeletalArray);

	AnimatorSkeletalArray();

private:
	inline void SetAnimationImpl(ID animationId, float blendTime)
	{
		m_animationId = animationId;
		m_t = 0;

		for (auto& index : m_keyFramesIndex)
		{
			index.s = 0;
			index.r = 0;
			index.t = 0;
		}

		for (auto& index : m_aabbKeyFrameIndex)
		{
			index = 0;
		}

		auto& animation = m_model3D->m_animations[m_animationId];
		m_tickDuration = animation.tickDuration;
		m_ticksPerSecond = animation.ticksPerSecond;
	}

public:

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

	virtual ID GetCurrentAnimationId() const override;

	virtual void Play(ID animationId, float blendTime) override;

	virtual void Serialize(Serializer* serializer);

	virtual void Deserialize(Serializer* serializer);

	virtual Handle<Serializable> Clone(Serializer* serializer) override;

	virtual void CleanUp();

	virtual Handle<ClassMetadata> GetMetadata(size_t sign);

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue);

	void Update(Scene* scene, float dt);
};


NAMESPACE_END