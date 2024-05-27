#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class API AnimPlayerLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimPlayerLayer, SERIALIZABLE_MEM_RAW);

	Animation*						m_animation;

	std::vector<KeyFramesIndex>		m_keyFramesIndex;
	std::vector<uint32_t>			m_aabbKeyFrameIndex;

	std::vector<KeyFramesIndex>		m_startKeyFrameIndex;
	std::vector<uint32_t>			m_startAABBKeyFrameIndex;

	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;
	float m_startTick = 0;
	float m_t = 0;

	bool m_needResetKeyFrameIndex = false;

protected:
	// Inherited via AnimLayer
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

public:
	virtual void Run(float dt) override;

private:
	void SetAnimationImpl(Animation* animation, float startTime, float endTime);
	void SetTimeImpl(float tick, float startTick, float tickDuration, float tickPerSecond);

public:
	// time in sec
	void SetAnimation(Animation* animation, float startTime, float endTime);

	// t in sec
	void SetCurrentTime(float t);
	void SetStartTime(float t);
	void SetEndTime(float t);
	void SetDuration(float duration);
	void SetTime(float currentTime, float startTime, float endTime, float duration);

};

NAMESPACE_END