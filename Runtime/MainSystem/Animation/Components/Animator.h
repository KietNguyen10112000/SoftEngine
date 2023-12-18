#pragma once

#include "AnimationComponent.h"

#include "Common/Base/AsyncTaskRunner.h"

#include "ANIMATION_TYPE.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class Animator : public AnimationComponent
{
public:
	struct Trigger
	{
		Handle<FunctionBase> callback;
		float percentTime;
		float padd;

		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(callback);
		}
	};

	Resource<AnimModel>	m_model3D;

protected:
	spinlock m_tiggersLock;
	bool m_padd[3];
	UnorderedLinkedList<Handle<FunctionBase>> m_triggers;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_triggers);
	}

public:
	inline Animator() : AnimationComponent(ANIMATION_TYPE::NONE) {};

	inline Animator(ANIMATION_TYPE type) : AnimationComponent(type) {};

	// return id of animation, return INVALID_ID if not found
	virtual ID FindAnimation(const String& name) = 0;

	virtual void GetAnimationsName(std::vector<String>& output) const = 0;

	// set duration of current animation
	virtual void SetDuration(float sec) = 0;

	// set duration of specified animation, so the duration will be used when the animation start playing
	virtual void SetDuration(float sec, ID animationId) = 0;

	// in sec, get duration of current animation
	virtual float GetDuration() const = 0;

	virtual ID GetCurrentAnimationId() const = 0;

	virtual void Play(ID animationId, float startTime, float endTime, float blendTime) = 0;

	template <typename Fn, typename... Args>
	inline ID AddTrigger(float percentTime, Fn fn, Args&&... args)
	{
		Trigger trigger = {};
		trigger.callback = MakeAsyncFunction(fn, std::forward<Args>(args)...);
		trigger.percentTime = percentTime;

		m_tiggersLock.lock();
		auto ret = m_triggers.Add(trigger);
		m_tiggersLock.unlock();

		return ret;
	}

	inline void RemoveTrigger(ID id)
	{
		m_tiggersLock.lock();
		m_triggers.Remove(id);
		m_tiggersLock.unlock();
	}

};

NAMESPACE_END