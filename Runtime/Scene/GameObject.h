#pragma once

#include "Core/Memory/Memory.h"
#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

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

private:
	Handle<MainComponent> m_mainComponents[MainSystemInfo::COUNT] = {};

	// external components
	Array<ComponentSlot> m_components = {};

	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_children;

	Scene* m_scene = nullptr;
	ID m_sceneId = INVALID_ID;

	String m_name;
	Transform m_transform;

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_mainComponents);
		tracer->Trace(m_components);
		tracer->Trace(m_parent);
		tracer->Trace(m_children);
	}

private:
	template <typename Comp>
	GameObject* AddMainComponent(const Handle<Comp>& component)
	{
		auto& slot = m_subSystemComponents[Comp::COMPONENT_ID];
		if (slot.Get() != nullptr)
		{
			return nullptr;
		}

		slot = component;
		component->m_object = this;
		//component->OnComponentAdded();

		return this;
	}

	auto FindComponentFromDtor(ComponentDtor dtor) const
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

public:
	// return null if object has one component has same type
	template <typename Comp>
	GameObject* AddComponent(const Handle<Comp>& component)
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			return AddMainComponent(component);
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
		if (AddComponent(comp))
		{
			return comp;
		}
		return nullptr;
	}
	
};

NAMESPACE_END