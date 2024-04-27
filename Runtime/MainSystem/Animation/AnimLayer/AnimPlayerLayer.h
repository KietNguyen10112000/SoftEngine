#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class AnimPlayerLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimPlayerLayer);

	Animation*						m_animation;

	std::vector<KeyFramesIndex>		m_keyFramesIndex;
	std::vector<uint32_t>			m_aabbKeyFrameIndex;

	std::vector<KeyFramesIndex>		m_startKeyFrameIndex;
	std::vector<uint32_t>			m_startAABBKeyFrameIndex;

	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;
	float m_startTick = 0;
	float m_t = 0;

protected:
	// Inherited via AnimLayer
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void Serialize(Serializer* serializer) override;

	void Deserialize(Serializer* serializer) override;

	void CleanUp() override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

public:
	virtual void Run(float dt) override;

private:
	void SetAnimationImpl(ID animationId, float startTime, float endTime);

public:
	void SetAnimation(ID animationId, float startTime, float endTime);

};

NAMESPACE_END