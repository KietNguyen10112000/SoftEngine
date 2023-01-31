#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/TemplateUtils/TemplateUtils.h"

#include "Math/Math.h"

#include "SubSystems/SubSystemInfo.h"

#include "Components/SubSystemComponent.h"

NAMESPACE_BEGIN

class GameObject : Traceable<GameObject>
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

	DefineHasClassMethod(OnCompentAdded);

public:
	struct EventCallback
	{
		void (*call)(const Handle<GameObject>&, void*);
		void* param;
	};

	enum EVENT
	{
		ADDED_TO_SCENE,

		COUNT
	};

private:
	// faster accessing
	Handle<SubSystemComponent> m_subSystemComponents[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	Array<ComponentSlot> m_components = {};

	std::Vector<EventCallback> m_events[EVENT::COUNT] = {};

protected:
	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_childs;

	ID m_id = -1;
	Transform m_transform = {};

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

	void TriggerEvent(const Handle<GameObject>& self, EVENT type)
	{
		assert(this == self.Get());

		auto& events = m_events[type];
		for (auto& e : events)
		{
			e.call(self, e.param);
		}
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
	ID AddEvent(EVENT type, EventCallback callback)
	{
		assert(type < EVENT::COUNT);
		m_events[type].push_back(callback);
		return 0;
	}

	void RemoveEvent(ID event)
	{
		
	}

};

NAMESPACE_END