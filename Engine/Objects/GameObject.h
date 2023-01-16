#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"

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

private:
	// faster accessing
	ComponentSlot m_subSystemComponents[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	Array<ComponentSlot> m_components = {};

protected:
	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_childs;

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
		auto& slot = m_subSystemComponents[Comp::s_id];
		if (slot.ptr.Get() != nullptr)
		{
			return nullptr;
		}

		//slot.dtor = GetDtor<Comp>();
		slot.ptr = component;
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
		if constexpr (std::is_base_of_v<SubSystemComponent<Comp>, Comp>)
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
		if constexpr (std::is_base_of_v<SubSystemComponent<Comp>, Comp>)
		{
			return StaticCast<Comp>(m_subSystemComponents[Comp::s_id].ptr);
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
		if constexpr (std::is_base_of_v<SubSystemComponent<Comp>, Comp>)
		{
			return (Comp*)(m_subSystemComponents[Comp::s_id].ptr.Get());
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

};

NAMESPACE_END