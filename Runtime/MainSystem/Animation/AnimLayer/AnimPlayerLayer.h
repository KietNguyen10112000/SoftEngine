#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class AnimPlayerLayer : public AnimLayer
{
protected:
	Animation*						m_animation;

	std::vector<KeyFramesIndex>		m_keyFramesIndex;
	std::vector<uint32_t>			m_aabbKeyFrameIndex;

	std::vector<KeyFramesIndex>		m_startKeyFrameIndex;
	std::vector<uint32_t>			m_startAABBKeyFrameIndex;

	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;
	float m_startTick = 0;
	float m_t = 0;

public:
	virtual void Run(float dt) override;

private:
	void SetAnimationImpl(ID animationId, float startTime, float endTime);

public:
	void SetAnimation(ID animationId, float startTime, float endTime);

};

NAMESPACE_END