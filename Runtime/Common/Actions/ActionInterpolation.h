#pragma once

#include "ActionBase.h"

#include "Math/Math.h"

#include "Common/Math/Function1D.h"

NAMESPACE_BEGIN

#ifdef _DEBUG

#define ActionInterpolation_CHECK_KEYFRAMES(keyframes)		\
assert(keyframes.size() >= 2);								\
for (size_t i = 0; i < keyframes.size() - 1; i++)			\
{															\
	assert(keyframes[i].time < keyframes[i + 1].time);		\
}

#else

#define ActionInterpolation_CHECK_KEYFRAMES(v) 

#endif // _DEBUG


template <typename T>
class ActionInterpolation : public ActionBase
{
public:
	template <typename TT>
	struct InterpolationFnStruct
	{
		inline static TT Fn(const TT& v1, const TT& v2, float s)
		{
			return Lerp(v1, v2, s);
		}
	};

	template <>
	struct InterpolationFnStruct<Transform>
	{
		inline static Transform Fn(const Transform& v1, const Transform& v2, float s)
		{
			Transform ret;
			ret.Scale()		= Lerp(v1.GetScale(), v2.GetScale(), s);
			ret.Rotation()	= SLerp(v1.GetRotation(), v2.GetRotation(), s);
			ret.Position()	= Lerp(v1.GetPosition(), v2.GetPosition(), s);
			return ret;
		}
	};

	template <>
	struct InterpolationFnStruct<Quaternion>
	{
		inline static Quaternion Fn(const Quaternion& v1, const Quaternion& v2, float s)
		{
			return SLerp(v1, v2, s);
		}
	};

public:
	struct KeyFrame
	{
		// control time function
		SharedPtr<Function1D> controlFunction = nullptr;
		T value = 0;
		float time = 0;

		KeyFrame(const T& value, float time) : value(value), time(time) {};
		KeyFrame(const T& value, float time, const SharedPtr<Function1D>& timeControlFunction) : controlFunction(timeControlFunction), value(value), time(time) {};
	};

private:
	std::vector<KeyFrame> m_keyFrames;
	T m_currentValue = {};
	float m_currentTime = 0;
	float m_timeEnd = 0;
	ID m_index = 0;

	std::function<void(const T&)> m_callback;

public:
	inline virtual RETURN_CODE Update(float dt) override
	{
		if (m_keyFrames.size() <= 1)
		{
			m_currentValue = m_keyFrames.back().value;
			return RETURN_CODE::FINISHED;
		}

		if (m_currentTime >= m_timeEnd)
		{
			return RETURN_CODE::FINISHED;
		}

		m_currentTime += dt;

		RETURN_CODE returnCode = RETURN_CODE::NONE;
		if (m_currentTime >= m_timeEnd)
		{
			if (m_timeEnd == m_keyFrames.back().time)
			{
				m_currentValue = m_keyFrames.back().value;
				if (m_callback)
				{
					m_callback(GetCurrentValue());
				}
				return RETURN_CODE::FINISHED;
			}

			m_currentTime = m_timeEnd;
			returnCode = RETURN_CODE::FINISHED;
		}

		while (m_index < m_keyFrames.size() && m_keyFrames[m_index].time < m_currentTime)
		{
			m_index++;
		}

		assert(m_index <= m_keyFrames.size());

		auto& k1 = m_keyFrames[m_index - 1];
		auto& k2 = m_keyFrames[m_index];

		auto s = (m_currentTime - k1.time) / (k2.time - k1.time);
		if (k1.controlFunction.get())
		{
			s = k1.controlFunction->Test(s);
		}

		m_currentValue = InterpolationFnStruct<T>::Fn(k1.value, k2.value, s);
		if (m_callback)
		{
			m_callback(GetCurrentValue());
		}
		
		return returnCode;
	}

	inline void AddKeyFrame(const KeyFrame& k)
	{
#ifdef _DEBUG
		if (!m_keyFrames.empty())
		{
			assert(k.time > m_keyFrames.back().time);
		}
#endif // _DEBUG

		m_keyFrames.push_back(k);
		m_timeEnd = k.time;
	}

	inline void AddKeyFrames(const std::vector<KeyFrame>& keyframes, float startTime = -1, float endTime = -1)
	{
		if (startTime < 0 && endTime < 0)
		{
			for (auto& k : keyframes)
			{
				AddKeyFrame(k);
			}

			return;
		}
		
		assert(startTime >= 0 && endTime >= startTime);

		ActionInterpolation_CHECK_KEYFRAMES(keyframes);

		ID startHigherIdx = INVALID_ID;
		auto startValue = ExtractKeyFrameValue(keyframes, startTime, &startHigherIdx);

		ID endHigherIdx = INVALID_ID;
		auto endValue = ExtractKeyFrameValue(keyframes, endTime, &endHigherIdx);

		assert(startHigherIdx != endHigherIdx);

		if (startTime != keyframes.front().time)
		{
			AddKeyFrame({ startValue,startTime });
		}

		auto endIdx = endHigherIdx == INVALID_ID ? keyframes.size() : endHigherIdx - 1;
		for (size_t i = startHigherIdx; i < endIdx; i++)
		{
			AddKeyFrame(keyframes[i]);
		}

		if (endTime != keyframes.back().time)
		{
			AddKeyFrame({ endValue,endTime });
		}
	}

	// callback on current value changed
	inline void SetCallback(const std::function<void()>& callback)
	{
		m_callback = callback;
	}

	inline void SetTimeEnd(float timeEnd)
	{
		m_timeEnd = timeEnd;
		assert(m_index == 0 && "SetTimeEnd() must be called right after Reset()!");
	}

	inline T GetCurrentValue() const
	{
		return m_currentValue;
	}

	inline void Reset()
	{
		m_keyFrames.clear();
		m_currentValue = {};
		m_currentTime = 0;
		m_timeEnd = 0;
		m_index = 0;
	}

	inline static auto New(const std::vector<KeyFrame>& keyframes)
	{
		auto ret = std::make_shared<ActionInterpolation<T>>();
		ret->AddKeyFrames(keyframes);
		return ret;
	}

	inline static auto New(const std::vector<KeyFrame>& keyframes, const std::function<void(const T&)>& callback)
	{
		auto ret = std::make_shared<ActionInterpolation<T>>();
		ret->AddKeyFrames(keyframes);
		ret->m_callback = callback;
		return ret;
	}

	inline static T ExtractKeyFrameValue(const std::vector<KeyFrame>& keyframes, float time, ID* outputIdx = nullptr)
	{
		ActionInterpolation_CHECK_KEYFRAMES(keyframes);

		auto& front = keyframes.front();
		auto& back = keyframes.back();

		if (front.time >= time)
		{
			if (outputIdx)
			{
				*outputIdx = 0;
			}
			return front.value;
		}

		if (back.time <= time)
		{
			if (outputIdx)
			{
				*outputIdx = INVALID_ID;
			}
			return back.value;
		}

		auto count = keyframes.size();
		for (size_t i = 0; i < count; i++)
		{
			if (keyframes[i].time > time)
			{
				if (outputIdx)
				{
					*outputIdx = i;
				}

				auto& k1 = keyframes[i - 1];
				auto& k2 = keyframes[i];
				auto s = (time - k1.time) / (k2.time - k1.time);
				return InterpolationFnStruct<T>::Fn(k1.value, k2.value, s);
			}
		}

		assert(0);
		return {};
	}

	inline static std::vector<KeyFrame> ExtractKeyFrames(const std::vector<KeyFrame>& keyframes, float startTime, float endTime)
	{
		assert(startTime >= 0 && endTime >= startTime);
		ActionInterpolation_CHECK_KEYFRAMES(keyframes);

		std::vector<KeyFrame> ret;

		ID startHigherIdx = INVALID_ID;
		auto startValue = ExtractKeyFrameValue(keyframes, startTime, &startHigherIdx);

		ID endHigherIdx = INVALID_ID;
		auto endValue = ExtractKeyFrameValue(keyframes, endTime, &endHigherIdx);

		assert(startHigherIdx != endHigherIdx);

		if (startTime != keyframes.front().time)
		{
			ret.push_back({ startValue,startTime });
		}

		auto endIdx = endHigherIdx == INVALID_ID ? keyframes.size() : endHigherIdx - 1;
		for (size_t i = startHigherIdx; i < endIdx; i++)
		{
			ret.push_back(keyframes[i]);
		}

		if (endTime != keyframes.back().time)
		{
			ret.push_back({ endValue,endTime });
		}

		return ret;
	}
};

#ifdef ActionInterpolation_CHECK_KEYFRAMES
#undef ActionInterpolation_CHECK_KEYFRAMES
#endif // ActionInterpolation_CHECK_KEYFRAMES


NAMESPACE_END