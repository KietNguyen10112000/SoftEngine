#pragma once

#include "AnimLayer.h"
#include "Common/Base/AsyncTaskRunner.h"

#ifdef PLUGIN_ALLOW_HOT_RELOAD
#include "Plugins/Plugin.h"
#endif

NAMESPACE_BEGIN

class API AnimPlayerLayer : public AnimLayer
{
public:
	class EventListener
	{
	private:
		friend class AnimPlayerLayer;

		Handle<FunctionBase> m_callback;

		ID m_callerCompId = INVALID_ID;

		float m_t = 0;
		uint32_t m_id = uint32_t(INVALID_ID);

#ifdef PLUGIN_ALLOW_HOT_RELOAD
		Plugin* m_ownedPlugin = nullptr;
#endif // PLUGIN_ALLOW_HOT_RELOAD

		TRACEABLE_FRIEND();
		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(m_callback);
		}

	public:
		inline auto& TriggerTick()
		{
			return m_t;
		}

	};

protected:
	SERIALIZABLE_CLASS(AnimPlayerLayer);

	friend class AnimTransitLayer;

	SharedPtr<Animation>			m_animation;

	std::vector<KeyFramesIndex>		m_keyFramesIndex;
	std::vector<KeyFramesIndex>		m_startKeyFrameIndex;

	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;
	float m_startTick = 0;
	float m_t = 0;

	bool m_needResetKeyFrameIndex = false;
	bool m_disableRootMotionScaling = false;
	bool m_disableRootMotionRotation = false;
	bool m_disableRootMotionTranslation = false;

	spinlock m_lock;
	Array<Handle<EventListener>> m_events = {};

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_events);
	}

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
	void SetAnimationImpl(const SharedPtr<Animation>& animation, float startTime, float endTime);
	void SetTimeImpl(float tick, float startTick, float tickDuration, float tickPerSecond);

	void SetCurrentTimeImpl(float t);
	void SetStartTimeImpl(float t);
	void SetEndTimeImpl(float t);
	void SetDurationImpl(float duration);
	void SetTimeImpl_(float currentTime, float startTime, float endTime, float duration);

public:
	// time in sec
	void SetAnimation(const SharedPtr<Animation>& animation, float startTime, float endTime);

	// t in sec
	void SetCurrentTime(float t);
	void SetStartTime(float t);
	void SetEndTime(float t);
	void SetDuration(float duration);
	void SetTime(float currentTime, float startTime, float endTime, float duration);

	inline float GetTime() const
	{
		return m_t / m_ticksPerSecond;
	}

public:
	template <ID MAIN_SYSTEM_ID, typename _Caller, typename Fn, typename... Args>
	inline auto AddPlayingListener(_Caller* caller, float t, Fn fn, Args&&... args)
	{
		auto listener = mheap::New<EventListener>();
		listener->m_callback = MakeAsyncFunction(fn, Handle<_Caller>(caller), std::forward<Args>(args)...);
		listener->m_callerCompId = MAIN_SYSTEM_ID;
		listener->m_t = t;
		
#ifdef PLUGIN_ALLOW_HOT_RELOAD
		listener->m_ownedPlugin = Plugin::GetInstance();
#endif // PLUGIN_ALLOW_HOT_RELOAD

		m_lock.lock();
		listener->m_id = m_events.size();
		m_events.Push(listener);
		m_lock.unlock();

		return listener;
	}

	template <typename _MainComponent, typename Fn, typename... Args>
	inline auto AddPlayingListener(_MainComponent* caller, float t, Fn fn, Args&&... args)
	{
		return AddPlayingListener<_MainComponent::COMPONENT_ID>(caller, t, fn, std::forward<Args>(args)...);
	}

	void RemoveListener(EventListener* listener);

	void SetEnableRootMotion(bool enableScaling, bool enableRotation, bool enableTranslation);

	static void MakeClipCut(std::vector<Transform>& localTransforms, AnimModel* model, Animation* animation, float tick);

};

NAMESPACE_END