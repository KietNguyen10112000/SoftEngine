#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Memory/SmartPointers.h"

#include "Math/Math.h"

#include "SubSystems/SubSystemInfo.h"

#include "Components/SubSystemComponent.h"

NAMESPACE_BEGIN

class GameObject : Traceable<GameObject>
{
public:
	using ComponentDtor = void(*)(void*);
	struct ComponentSlot
	{
		void (*dtor)(void*) = 0;
		SharedPtr<void> ptr;
	};


private:
	// faster accessing
	ComponentSlot m_subSystemComponents[SubSystemInfo::SUBSYSTEMS_COUNT] = {};

	std::Vector<ComponentSlot> m_components = {};

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
	GameObject* AddSubSystemComponent(const SharedPtr<Comp>& component)
	{
		auto& slot = m_subSystemComponents[Comp::s_id];
		if (slot.ptr.get() != nullptr)
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
				return v.dtor == dtor;
			}
		);
		return it;
	}

	template <typename Comp>
	GameObject* AddNormalComponent(const SharedPtr<Comp>& component)
	{
		ComponentDtor dtor = GetDtor<Comp>();	
		auto it = FindComponentFromDtor(dtor);

		if (it != m_components.end())
		{
			return nullptr;
		}

		m_components.push_back({ dtor, component });

		return this;
	}

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_parent);
		tracer->Trace(m_childs);
	}

public:
	// return null if object has one component has same type
	template <typename Comp>
	GameObject* AddComponent(const SharedPtr<Comp>& component)
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
	SharedPtr<Comp> NewComponent(Args&&... args)
	{
		auto comp = MakeShared<Comp>(std::forward<Args>(args)...);
		AddComponent(comp);
		return comp;
	}

	template <typename Comp>
	SharedPtr<Comp> GetComponent() const
	{
		if constexpr (std::is_base_of_v<SubSystemComponent<Comp>, Comp>)
		{
			return SharedPtrStaticCast<Comp>(m_subSystemComponents[Comp::s_id].ptr);
		}
		else
		{
			ComponentDtor dtor = GetDtor<Comp>();
			auto it = FindComponentFromDtor(dtor);

			if (it != m_components.end())
			{
				return SharedPtrStaticCast<Comp>(it->ptr);
			}
			return nullptr;
		}
	}

};

NAMESPACE_END