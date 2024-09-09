#pragma once

#include "AnimLayer.h"
#include "Common/Base/AsyncTaskRunner.h"

#ifdef PLUGIN_ALLOW_HOT_RELOAD
#include "Plugins/Plugin.h"
#endif

#include <queue>

NAMESPACE_BEGIN

class API AnimTransitLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimTransitLayer);

public:
	class EventListener
	{
	private:
		friend class AnimTransitLayer;

		Handle<FunctionBase> m_callback;

		uint32_t m_callerCompId = uint32_t(INVALID_ID);
		uint32_t m_id = uint32_t(INVALID_ID);

#ifdef PLUGIN_ALLOW_HOT_RELOAD
		Plugin* m_ownedPlugin = nullptr;
#endif // PLUGIN_ALLOW_HOT_RELOAD

		TRACEABLE_FRIEND();
		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(m_callback);
		}
	};

	struct TransitDirection
	{
		enum DIRECTION
		{
			// current animation stoped, next animation is still playing
			FORWARD,

			// next animation stoped, current animation is still playing
			BACKWARD,

			// both animations stoped
			FIXED,
		};
	};

	struct FadeState
	{
		SharedPtr<Animation> animation = nullptr;
		TransitDirection::DIRECTION direction = TransitDirection::FORWARD;
		float fadeTime;
		float startTime;
		float endTime;
	};

	spinlock m_lock;
	bool m_padd[7];
	Array<Handle<EventListener>> m_events = {};

	std::queue<FadeState> m_queue;

	AnimLayer* m_input = nullptr;

	std::vector<Transform> m_lastLocalTransforms;
	size_t m_lastUnSetCctIterationCount = INVALID_ID;

	float m_transitTime = 0;
	float m_transitTotalTime = 0;

	//FadeState m_lastFadeState = {};
	FadeState m_curFadeState = {};

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_events);
	}

private:
	void OnEndTransit();

	void CopyLastLocalTransforms(const std::vector<Transform>& lastLocalTransform);

protected:
	// Inherited via AnimLayer
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void Initialize() override;
	void PrevRun(float dt) override;
	void Run(float dt) override;

public:
	virtual AnimLayer* GetOutput() override;

	void SetInput(AnimLayer* l);
	void FadeTo(TransitDirection::DIRECTION direction, float fadeTime, const SharedPtr<Animation>& animation, float startTime, float endTime);
	void QueuedFadeTo(TransitDirection::DIRECTION direction, float fadeTime, const SharedPtr<Animation>& animation, float startTime, float endTime);

	bool IsEndFade() const;

public:
	template <ID MAIN_SYSTEM_ID, typename _Caller, typename Fn, typename... Args>
	inline auto AddEndTransitListener(_Caller* caller, Fn fn, Args&&... args)
	{
		auto listener = mheap::New<EventListener>();
		listener->m_callback = MakeAsyncFunction(fn, Handle<_Caller>(caller), std::forward<Args>(args)...);
		listener->m_callerCompId = uint32_t(MAIN_SYSTEM_ID);

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
	inline auto AddEndTransitListener(_MainComponent* caller, Fn fn, Args&&... args)
	{
		return AddEndTransitListener<_MainComponent::COMPONENT_ID>(caller, fn, std::forward<Args>(args)...);
	}

	void RemoveListener(EventListener* listener);

	inline AnimLayer* GetInput()
	{
		return m_input->GetOutput();
	}
};

NAMESPACE_END