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

	std::vector<KeyFramesIndex> m_blendKeyFramesIndex;

	std::vector<uint32_t> m_blendAabbKeyFrameIndex;

	/*ID m_animationId = 0;
	float m_tickDuration;
	float m_ticksPerSecond;
	float m_t = 0;*/

	AnimationTrack* m_currentAnimTrack = nullptr;
	AnimationTrack* m_blendingAnimTrack = nullptr;

	DeferredBuffer<AnimationTrack> m_currentAnimTrackBuffer;
	DeferredBuffer<AnimationTrack> m_blendingAnimTrackBuffer;

	float m_t = 0;
	float m_blendTime = 0;
	float m_blendCurTime = 0;
	float m_tBlend = 0;

	float m_startTransitTick = 0;
	float m_transitStartTime = 0;

	bool m_paused = false;
	bool m_padd[7];

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
	inline void SetAnimationImpl(float startTransitTime, float blendTime, float startTime)
	{
		startTransitTime = startTransitTime * m_currentAnimTrack->ticksPerSecond;

		if (startTransitTime > 0)
		{
			startTransitTime = std::clamp(startTransitTime, m_currentAnimTrack->startTick, m_currentAnimTrack->startTick + m_currentAnimTrack->tickDuration);
		}

		m_startTransitTick = startTransitTime - m_currentAnimTrack->startTick;

		if (blendTime != 0)
		{
			/*m_blendingAnimTrack = (decltype(m_blendingAnimTrack))m_blendingAnimTrackBuffer.Read();
			m_blendTime = blendTime;
			m_blendCurTime = 0;
			m_tBlend = 0;

			std::memcpy(m_blendKeyFramesIndex.data(), m_blendingAnimTrack->startKeyFramesIndex.data(),
				m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

			std::memcpy(m_blendAabbKeyFrameIndex.data(), m_blendingAnimTrack->startAABBKeyFrameIndex.data(),
				m_aabbKeyFrameIndex.size() * sizeof(uint32_t));*/

			m_blendingAnimTrack = (decltype(m_blendingAnimTrack))m_currentAnimTrack;
			m_blendTime = blendTime;
			m_blendCurTime = 0;
			m_tBlend = m_t;

			std::memcpy(m_blendKeyFramesIndex.data(), m_keyFramesIndex.data(),
				m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

			std::memcpy(m_blendAabbKeyFrameIndex.data(), m_aabbKeyFrameIndex.data(),
				m_aabbKeyFrameIndex.size() * sizeof(uint32_t));

			m_transitStartTime = startTime;

			if (m_startTransitTick < 0)
			{
				m_currentAnimTrack = (decltype(m_currentAnimTrack))m_blendingAnimTrackBuffer.Read();

				m_t = startTime * m_currentAnimTrack->ticksPerSecond - m_currentAnimTrack->startTick;

				std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
					m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

				std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
					m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
			}
		}
		else
		{
			m_blendTime = 0;
			m_blendingAnimTrack = nullptr;

			m_currentAnimTrack = (decltype(m_currentAnimTrack))m_currentAnimTrackBuffer.Read();

			m_t = startTime * m_currentAnimTrack->ticksPerSecond - m_currentAnimTrack->startTick;

			std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(), 
				m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

			std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(), 
				m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
		}
	}

	inline void SetTimeImpl(float t)
	{
		t *= m_currentAnimTrack->ticksPerSecond;
		t = std::clamp(t, m_currentAnimTrack->startTick, m_currentAnimTrack->startTick + m_currentAnimTrack->tickDuration);
		
		m_t = t; //- m_currentAnimTrack->startTick;

		std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
	}

	void UpdateNoBlend(Scene* scene);

	void UpdateBlend(Scene* scene, float dt);

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

	virtual void Play(float startTransitTime, ID animationId, float startTime, float beginTime, float endTime, float blendTime) override;

	virtual void SetPause(bool pause) override;

	virtual void SetTime(float t) override;

	virtual void Serialize(Serializer* serializer);

	virtual void Deserialize(Serializer* serializer);

	virtual Handle<Serializable> Clone(Serializer* serializer) override;

	virtual void CleanUp();

	virtual Handle<ClassMetadata> GetMetadata(size_t sign);

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue);

	void Update(Scene* scene, float dt);
};


NAMESPACE_END