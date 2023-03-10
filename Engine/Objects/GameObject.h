#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Structures.h"
#include "Core/TemplateUtils/TemplateUtils.h"

#include "Math/Math.h"

#include "SubSystems/SubSystemInfo.h"

#include "Components/SubSystemComponent.h"

#include "Scene/Event/EventListener.h"
#include "Scene/Event/EventListenerContainer.h"

NAMESPACE_BEGIN

class GameObject final : Traceable<GameObject>
{
public:
	using ComponentDtor = void(*)(void*);
	struct ComponentSlot : Traceable<ComponentSlot>
	{
		ID identifier = 0;
		Handle<void> ptr;

	private:
		TRACEABLE_FRIEND();
		void Trace(Tracer* tracer)
		{
			tracer->Trace(ptr);
		}
	};

	friend class Scene;
	friend class DynamicLayer;
	friend class MultipleDynamicLayersScene;

	DefineHasClassMethod(OnCompentAdded);

public:
	enum EVENT
	{
		ADDED_TO_SCENE,

		COUNT
	};

private:
	// the scene, absolutely, live to the end of object-live, no need managed ptr
	// when the scene destroy, it sets this field to null
	Scene* m_scene = nullptr;

	// faster accessing
	Handle<SubSystemComponent> m_subSystemComponents[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	Array<ComponentSlot> m_components = {};

	EventListenerContainer m_eventListeners;

private:
	// >>> scene's control members 
	ID m_uid = INVALID_ID;
	ID m_sceneId = INVALID_ID;
	ID m_sceneDynamicId = INVALID_ID;
	ID m_aabbQueryId = INVALID_ID;

	AABox m_aabb = {};
	// <<< scene's control members 

	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_childs;

protected:
	DeferredBuffer<Transform> m_transform = {};

private:
	template <typename T>
	ComponentDtor GetDtor() const
	{
		return [](void* ptr)
		{
			((T*)ptr)->~T();
		};
	}

	template <typename Comp>
	GameObject* AddSubSystemComponent(const Handle<Comp>& component)
	{
		auto& slot = m_subSystemComponents[Comp::COMPONENT_ID];
		if (slot.Get() != nullptr)
		{
			return nullptr;
		}

		//slot.dtor = GetDtor<Comp>();
		slot = component;
		component->OnComponentAdded(this);
		return this;
	}

	inline auto FindComponentFromDtor(ComponentDtor dtor) const
	{
		auto it = std::find_if(m_components.begin(), m_components.end(), [=](const ComponentSlot& v) -> bool
			{
				return v.identifier == (ID)dtor;
			}
		);
		return it;
	}

	template <typename Comp>
	GameObject* AddNormalComponent(const Handle<Comp>& component)
	{
		ComponentDtor dtor = GetDtor<Comp>();	
		auto it = FindComponentFromDtor(dtor);

		if (it != m_components.end())
		{
			return nullptr;
		}

		ComponentSlot slot = {};
		slot.identifier = (ID)dtor;
		slot.ptr = component;
		m_components.Push(slot);

		if constexpr (Has_OnCompentAdded<Comp>::value)
		{
			component->OnCompentAdded(this);
		}

		return this;
	}

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_subSystemComponents);
		tracer->Trace(m_components);
		tracer->Trace(m_parent);
		tracer->Trace(m_childs);
	}

public:
	// return null if object has one component has same type
	template <typename Comp>
	GameObject* AddComponent(const Handle<Comp>& component)
	{
		if constexpr (std::is_base_of_v<SubSystemComponent, Comp>)
		{
			return AddSubSystemComponent(component);
		}
		else
		{
			return AddNormalComponent(component);
		}
	}

	// make new component inside object
	template <typename Comp, typename... Args>
	Handle<Comp> NewComponent(Args&&... args)
	{
		auto comp = mheap::New<Comp>(std::forward<Args>(args)...);
		AddComponent(comp);
		return comp;
	}

	template <typename Comp>
	Handle<Comp> GetComponent() const
	{
		if constexpr (std::is_base_of_v<SubSystemComponent, Comp>)
		{
			return StaticCast<Comp>(m_subSystemComponents[Comp::COMPONENT_ID]);
		}
		else
		{
			ComponentDtor dtor = GetDtor<Comp>();
			auto it = FindComponentFromDtor(dtor);

			if (it != m_components.end())
			{
				return StaticCast<Comp>(it->ptr);
			}
			return nullptr;
		}
	}

	template <typename Comp>
	Comp* GetComponentRaw() const
	{
		if constexpr (std::is_base_of_v<SubSystemComponent, Comp>)
		{
			return (Comp*)(m_subSystemComponents[Comp::COMPONENT_ID].Get());
		}
		else
		{
			ComponentDtor dtor = GetDtor<Comp>();
			auto it = FindComponentFromDtor(dtor);

			if (it != m_components.end())
			{
				return (Comp*)(it->ptr.Get());
			}
			return nullptr;
		}
	}

public:
	inline void AddEventListener(EVENT type, const Handle<EventListener>& listener)
	{
		assert(type < EVENT::COUNT);

		// 1 listener belongs to 1 event
		// 1 event contains many listeners
		assert(listener->m_id == 0);

		m_eventListeners.Add(type, listener);
	}

	inline void RemoveEventListener(const Handle<EventListener>& listener)
	{
		m_eventListeners.Remove(listener);
	}

	inline void InvokeEvent(const Handle<GameObject>& self, EVENT type)
	{
		assert(this == self.Get());

		const auto temp = type;
		m_eventListeners.m_listeners.ForEach([=](const Handle<EventListener>& listener)
			{
				if (listener->m_type == temp)
				{
					listener->OnInvoke(m_scene);
				}
			}
		);
	}

	inline void InvokeEvent(EVENT type)
	{
		InvokeEvent(this, type);
	}

public:
	inline const auto& GetAABB() const
	{
		return m_aabb;
	}


};

NAMESPACE_END