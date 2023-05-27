#pragma once

#include "Sprite.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

// frame by frame animation
class AnimatedSprites
{
public:
	using AnimationDoneCallback = bool (*)(AnimatedSprites*, void*);

	std::Vector<Sprite> m_spriteFrames;

	//
	// | Animation 1 frame count | duration | frame index 1 | frame index 2 | ... | Animation 2 frame count | ...
	//
	std::Vector<size_t> m_animations;

	size_t m_curAnimIdx = 0;
	size_t m_curAnimIdxBegin = 0;
	size_t m_curAnimIdxEnd = 0;
	float  m_sumDt = 0;
	float  m_curAnimDtPerFrame = 0;
	size_t m_curAnimID = INVALID_ID;
	size_t m_curSpriteFrameIdx = 0;

	AnimationDoneCallback m_doneCallback = nullptr;
	void* m_userPtr = nullptr;

private:
	inline ID GetAnimationID()
	{
		return m_animations.size();
	}

public:
	// return sprite frame ID that can be grouped as array then pass to MakeAnimation()
	inline ID LoadSpriteFrame(String path, const AARect& rect = {}, const Transform2D& transform = {})
	{
		m_spriteFrames.push_back({});
		auto& back = m_spriteFrames.back();
		back.Initialize(path, rect, transform);
		return m_spriteFrames.size() - 1;
	}

	// return animation id that can be passed to SetAnimation()
	inline ID MakeAnimation(ID* spriteFramesID, size_t count, float duration)
	{
		auto ret = GetAnimationID();

		size_t packFloat = 0;
		*((float*)&packFloat) = duration / (float)count;

		m_animations.push_back(count);
		m_animations.push_back(packFloat);
		m_animations.insert(m_animations.end(), spriteFramesID, spriteFramesID + count);
		m_animations.push_back(count);

		return ret;
	}

	// callback is null or return true if repeat animation again
	inline void SetAnimation(ID id, AnimationDoneCallback callback = 0, void* userPtr = 0)
	{
		auto count = m_animations[id];
		assert(id + count < m_animations.size());
		assert(m_animations[id + count + 2] == count);

		m_curAnimID = id;
		m_curAnimDtPerFrame = *((float*)&m_animations[id + 1]);
		m_curAnimIdxBegin = id + 2;
		m_curAnimIdxEnd = m_curAnimIdxBegin + count;
		m_curAnimIdx = m_curAnimIdxBegin;
		m_sumDt = 0;
		m_curSpriteFrameIdx = m_animations[m_curAnimIdx];

		m_doneCallback = callback;
		m_userPtr = userPtr;
	}

	inline void Play(float dt)
	{
		if (m_curAnimID == INVALID_ID) return;

		m_sumDt += dt;
		size_t count = (size_t)std::floor(m_sumDt / m_curAnimDtPerFrame);
		m_sumDt -= count * m_curAnimDtPerFrame;
		m_curAnimIdx += count;

		if (m_curAnimIdx >= m_curAnimIdxEnd)
		{
			// end animation
			bool repeat = m_doneCallback ? m_doneCallback(this, m_userPtr) : true;
			m_curAnimIdx = m_curAnimIdxEnd - 1;

			if (repeat)
			{
				SetAnimation(m_curAnimID, m_doneCallback, m_userPtr);
			}
		}

		m_curSpriteFrameIdx = m_animations[m_curAnimIdx];
	}

	inline auto& GetSpriteFrame(ID id)
	{
		return m_spriteFrames[id];
	}

	inline auto& GetCurrentSpriteFrame()
	{
		assert(!m_spriteFrames.empty());
		return m_spriteFrames[m_curSpriteFrameIdx];
	}

};

NAMESPACE_END