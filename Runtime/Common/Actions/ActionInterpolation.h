#pragma once

#include "ActionBase.h"

#include "Math/Math.h"

#include "Common/Math/Function1D.h"

NAMESPACE_BEGIN

template <typename T>
class ActionInterpolation : public ActionBase
{
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

		if (m_currentTime >= m_timeEnd)
		{
			m_currentValue = m_keyFrames.back().value;
			return RETURN_CODE::FINISHED;
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
		
		return RETURN_CODE::NONE;
	}

	inline void AddKeyFrame(const KeyFrame& k)
	{
		if (!m_keyFrames.empty())
		{
			assert(k.time > m_keyFrames.back().time);
		}

		m_keyFrames.push_back(k);
		m_timeEnd = k.time;
	}

	inline void AddKeyFrames(const std::vector<KeyFrame>& keyframes)
	{
		for (auto& k : keyframes)
		{
			AddKeyFrame(k);
		}
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

};

NAMESPACE_END