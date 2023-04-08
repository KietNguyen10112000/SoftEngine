#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Structures.h"
#include "Core/TemplateUtils/TemplateUtils.h"

#include "Math/Math.h"

#include "SubSystems/SubSystemInfo.h"

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

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

	friend class GameObjectDirectAccessor;
	friend class Scene;
	friend class DynamicLayer;
	friend class MultipleDynamicLayersScene;
	friend class SubSystem;
	friend class SubSystemMergingUnit;
	friend class Script;

	DefineHasClassMethod(OnCompentAdded);
	DefineHasClassMethod(OnCompentRemoved);

public:
	enum EVENT
	{
		ADDED_TO_SCENE,
		REMOVED_FROM_SCENE,

		COUNT
	};

private:
	// the scene, absolutely, live to the end of object-live, no need managed ptr
	// when the scene destroy, it sets this field to null
	Scene* m_scene = nullptr;

	// faster accessing
	Handle<SubSystemComponent> m_subSystemComponents[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	// count all descension child object components (include this object's component)
	int						   m_subSystemCompCounts[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	// store sub system root object id
	ID						   m_subSystemId		[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	// store number of iteration that sub system process this obj
	std::atomic<size_t>		   m_subSystemProcessCount[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};

	// external components
	Array<ComponentSlot> m_components = {};

	EventListenerContainer m_eventListeners;

private:
	// >>> scene's control members 
	ID m_uid = INVALID_ID;
	ID m_sceneId = INVALID_ID;
	ID m_sceneDynamicId = INVALID_ID;
	ID m_aabbQueryId = INVALID_ID;

	// global aabb, this aabb bound this object and its children
	AABox m_aabb = {};
	// <<< scene's control members 

	// num components flush data to this obj in 1 iteration
	std::atomic<size_t> m_numBranchCount;
	size_t m_numBranch;
	ID m_mainComponent = INVALID_ID;

	std::atomic<bool> m_isBranched;
	bool m_isNeedRefresh = false;
	bool m_padding[2];

	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_children;

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

#define TRACE_TO_ROOT(invokeFunc)						\
auto it = this;											\
while (it)												\
{														\
	invokeFunc;											\
	it = it->m_parent.Get();							\
}

	template <typename Comp>
	GameObject* AddSubSystemComponent(const Handle<Comp>& component)
	{
		auto& slot = m_subSystemComponents[Comp::COMPONENT_ID];
		if (slot.Get() != nullptr)
		{
			return nullptr;
		}

		TRACE_TO_ROOT(it->m_subSystemCompCounts[Comp::COMPONENT_ID]++);

		//slot.dtor = GetDtor<Comp>();
		slot = component;
		component->m_object = this;
		component->OnComponentAdded();

		if (!component->IsConflict())
		{
			return this;
		}

		if (m_mainComponent == INVALID_ID && SubSystemComponentId::PRIORITY[Comp::COMPONENT_ID] != -1)
		{
			m_mainComponent = Comp::COMPONENT_ID;
			m_subSystemComponents[Comp::COMPONENT_ID]->SetAsMain();
		}

		if (m_mainComponent != INVALID_ID 
			&& SubSystemComponentId::PRIORITY[m_mainComponent] > SubSystemComponentId::PRIORITY[Comp::COMPONENT_ID])
		{
			m_subSystemComponents[m_mainComponent]->SetAsExtra();
			m_mainComponent = Comp::COMPONENT_ID;
			m_subSystemComponents[Comp::COMPONENT_ID]->SetAsMain();
		}

		//if (component->IsConflict())
		//{
		//	m_numBranch++;
		//}

		m_numBranch++;

		return this;
	}

#undef TRACE_TO_ROOT

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

	/*template <typename Comp>
	GameObject* RemoveSubSystemComponent()
	{
		auto& slot = m_subSystemComponents[Comp::COMPONENT_ID];

		if (slot.Get() != nullptr)
		{
			slot->OnCompentRemoved(this);
		}

		if (slot->IsConflict())
		{
			m_numBranch--;
		}

		slot = nullptr;

		if (m_mainComponent == Comp::COMPONENT_ID)
		{
			size_t newMainId = INVALID_ID;
			size_t priority = -1;
			for (auto& compId : SubSystemComponentId::PROCESS_DATA_COMPONENTS)
			{
				auto comp = m_subSystemComponents[compId].Get();
				if (comp && priority > SubSystemComponentId::PRIORITY[compId])
				{
					priority = SubSystemComponentId::PRIORITY[compId];
					newMainId = compId;
				}
			}

			m_mainComponent = newMainId;
			if (m_mainComponent != INVALID_ID)
			{
				m_subSystemComponents[m_mainComponent]->SetAsMain(this);
			}
		}

		return this;
	}

	template <typename Comp>
	GameObject* RemoveNormalComponent()
	{
		ComponentDtor dtor = GetDtor<Comp>();
		auto it = FindComponentFromDtor(dtor);

		if constexpr (Has_OnCompentRemoved<Comp>::value)
		{
			if (it != m_components.end())
			{
				component->OnCompentRemoved(this);
			}
		}

		m_components.Remove(it);

		return this;
	}*/

	void InvokeAddRootComponentToSubSystem(SubSystemComponent* comp, const ID id);
	void InvokeRemoveRootComponentFromSubSystem(SubSystemComponent* comp, const ID id);

#define DEF_INVOKE_FUNC(name, invoke1, invoke2)									\
inline void name()																\
{																				\
	for (size_t i = 0; i < m_children.Size(); i++)								\
	{																			\
		m_children[i]->name();													\
	}																			\
	for (size_t i = 0; i < SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT; i++)		\
	{																			\
		auto& comp = m_subSystemComponents[i];									\
		if (comp.Get())															\
		{																		\
			invoke2;															\
			comp->invoke1();													\
		}																		\
	}																			\
}

	DEF_INVOKE_FUNC(
		InvokeOnComponentAddedToScene,
		OnComponentAddedToScene,
		if (SubSystemComponentId::IS_USE_ROOT_OBJECTS[i])
		{
			InvokeAddRootComponentToSubSystem(comp.Get(), i);
		}
	);

	DEF_INVOKE_FUNC(
		InvokeOnComponentRemovedFromScene,
		OnComponentRemovedFromScene,
		if (SubSystemComponentId::IS_USE_ROOT_OBJECTS[i])
		{
			InvokeRemoveRootComponentFromSubSystem(comp.Get(), i);
		}
	);

#undef DEF_INVOKE_FUNC

	inline bool IsRootObject()
	{
		return m_parent.IsNull();
	}

	inline void MergeSubSystemComponentsData()
	{
		assert(IsRootObject());

		for (auto& child : m_children)
		{
			child->MergeSubSystemComponentsData();
		}

		for (auto& compId : SubSystemComponentId::PROCESS_DATA_COMPONENTS)
		{
			if (compId == m_mainComponent) continue;

			auto comp = m_subSystemComponents[compId].Get();
			if (comp)
			{
				comp->ResolveConflict();
			}
		}

		m_numBranchCount.store(m_numBranch);
		m_isBranched.store(false);
	}

	template <typename Func>
	void ForEachSubSystemComponents(Func func)
	{
		for (auto& v : m_subSystemComponents)
		{
			if (!v.IsNull()) func(v);
		}
	}

	template <typename Func>
	inline void ForEachChildren(Func func)
	{
		for (auto& child : m_children)
		{
			func(child.Get());
		}
	}

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_subSystemComponents);
		tracer->Trace(m_components);
		tracer->Trace(m_parent);
		tracer->Trace(m_children);
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

	/*template <typename Comp>
	GameObject* RemoveComponent()
	{
		if constexpr (std::is_base_of_v<SubSystemComponent, Comp>)
		{
			return RemoveSubSystemComponent<Comp>();
		}
		else
		{
			return RemoveNormalComponent<Comp>();
		}
	}*/

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

	template <typename Func>
	inline static void PostTraversal(GameObject* obj, Func func)
	{
		for (auto& child : obj->m_children)
		{
			PostTraversal(child.Get(), func);
		}

		func(obj);
	}

public:
	inline const auto& GetAABB() const
	{
		return m_aabb;
	}

	inline auto GetScene() const
	{
		return m_scene;
	}

	inline void ScheduleRefresh()
	{
		m_isNeedRefresh = true;
	}

	inline GameObject* GetRoot()
	{
		auto it = this;
		while (it)
		{
			if (it->IsRootObject()) return it;
			it = it->m_parent.Get();
		}

		// unreachable
		assert(0);
	}


};

NAMESPACE_END