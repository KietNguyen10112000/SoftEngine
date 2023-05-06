#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/DeferredBuffer.h"
#include "Core/Structures/Structures.h"
#include "Core/TemplateUtils/TemplateUtils.h"
#include "Core/Random/Random.h"

#include "Math/Math.h"

//#include "Components2D/Rendering/Rendering2D.h"
#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

#include "SubSystems2D/SubSystemInfo2D.h"

NAMESPACE_BEGIN

class GameObject2D final : Traceable<GameObject2D>
{
public:
	enum TYPE
	{
		STATIC,
		DYNAMIC,
		KINEMATIC,
		GHOST
	};

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

	friend class GameObjectDirectAccessor2D;
	friend class Scene2D;
	friend class UniqueGridScene2D;
	friend class SubSystem2D;
	friend class Script2D;
	friend class Rendering2D;
	friend class RenderingSystem2D;
	friend class PhysicsSystem2D;
	friend class ScriptSystem2D;

	DefineHasClassMethod(OnCompentAdded);
	DefineHasClassMethod(OnCompentRemoved);

private:
	Scene2D* m_scene = nullptr;

	// >>> scene's control members 
	ID m_uid = INVALID_ID;
	ID m_sceneId = INVALID_ID;
	ID m_sceneDynamicId = INVALID_ID;
	ID m_aabbQueryId = INVALID_ID;

	// local aabb, with root object m_aabb == m_globalAABB
	AARect						 m_aabb = {};

	// global aabb
	AARect						m_globalAABB = {};

	Transform2D					m_cachedTransform;
	Transform2D					m_globalTransform;
	int							m_queried = 0;
	// <<< scene's control members 

protected:
	//Handle<Script2D>			m_script;
	//Handle<Rendering2D>			m_rendering;

	Handle<SubSystemComponent2D>	m_subSystemComponents[SubSystemInfo2D::INDEXED_SUBSYSTEMS_COUNT] = {};
	ID								m_subSystemID[SubSystemInfo2D::INDEXED_SUBSYSTEMS_COUNT];
	Array<ComponentSlot>			m_components = {};

	Handle<GameObject2D>			m_parent = nullptr;
	Array<Handle<GameObject2D>>		m_children;

	TYPE							m_type = TYPE::DYNAMIC;

	Transform2D						m_transform;
	String							m_name;

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


#undef TRACE_TO_ROOT

	void InvokeAddRootComponentToSubSystem(SubSystemComponent2D* comp, const ID id);
	void InvokeRemoveRootComponentFromSubSystem(SubSystemComponent2D* comp, const ID id);

#define DEF_INVOKE_FUNC(name, invoke1, invoke2)									\
inline void name()																\
{																				\
	for (size_t i = 0; i < m_children.Size(); i++)								\
	{																			\
		m_children[i]->name();													\
	}																			\
	for (size_t i = 0; i < SubSystemInfo2D::INDEXED_SUBSYSTEMS_COUNT; i++)		\
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
		if (SubSystemComponentId2D::IS_USE_ROOT_OBJECTS[i])
		{
			InvokeAddRootComponentToSubSystem(comp.Get(), i);
		}
	);

	DEF_INVOKE_FUNC(
		InvokeOnComponentRemovedFromScene,
		OnComponentRemovedFromScene,
		if (SubSystemComponentId2D::IS_USE_ROOT_OBJECTS[i])
		{
			InvokeRemoveRootComponentFromSubSystem(comp.Get(), i);
		}
	);

#undef DEF_INVOKE_FUNC

	inline bool IsRootObject()
	{
		return m_parent.IsNull();
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
		//tracer->Trace(m_script);
		//tracer->Trace(m_rendering);
		tracer->Trace(m_subSystemComponents);
		tracer->Trace(m_components);
		tracer->Trace(m_parent);
		tracer->Trace(m_children);
	}

	inline bool IsMoved()
	{
		return m_transform != m_cachedTransform;
	}

	inline bool IsMovedRecursive()
	{
		for (auto& child : m_children)
		{
			if (child->IsMovedRecursive())
			{
				return true;
			}
		}

		return IsMoved();
	}

	template <typename Func>
	void ForEachSubSystemComponents(Func func)
	{
		for (auto& v : m_subSystemComponents)
		{
			if (!v.IsNull()) func(v);
		}
	}

	inline void RecalculateAABB()
	{
		Vec2 temp[4];
		PostTraversalRoundedPositionTransform(
			this,
			[](GameObject2D* obj, const Transform2D& globalTransform)
			{
				bool first = true;
				AARect localAABB;
				obj->ForEachSubSystemComponents(
					[&](Handle<SubSystemComponent2D>& comp)
					{
						if (first)
						{
							localAABB = comp->GetLocalAABB();
							first = false;
							return;
						}

						auto aabb = comp->GetLocalAABB();
						localAABB.Joint(aabb);
					}
				);

				obj->ForEachChildren(
					[&](GameObject2D* child)
					{
						localAABB.Joint(child->m_aabb);
					}
				);

				obj->m_globalAABB = localAABB;
				obj->m_globalAABB.Transform(globalTransform.ToTransformMatrix());

				localAABB.Transform(obj->Transform().ToTransformMatrix());
				obj->m_aabb = localAABB;

				obj->m_globalTransform = globalTransform;
				obj->m_globalTransform.Translation().Round();

				//obj->Transform().Translation().Round();
				obj->m_cachedTransform = obj->m_transform;
			}
		);
	}

	template <typename Comp>
	GameObject2D* AddSubSystemComponent(const Handle<Comp>& component)
	{
		auto& slot = m_subSystemComponents[Comp::COMPONENT_ID];
		if (slot.Get() != nullptr)
		{
			return nullptr;
		}

		slot = component;
		component->m_object = this;
		component->OnComponentAdded();

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
	GameObject2D* AddNormalComponent(const Handle<Comp>& component)
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
	GameObject2D(const TYPE& type)
	{
		m_type = type;
		for (auto& v : m_subSystemID)
		{
			v = INVALID_ID;
		}
	}

	/*inline void SetScript(const Handle<Script2D>& script)
	{
		m_script = script;
		SubSystemComponent2D* comp = (SubSystemComponent2D*)m_script.Get();
		comp->m_object = this;
	}

	inline void SetRendering(const Handle<Rendering2D>& rendering)
	{
		m_rendering = rendering;
		SubSystemComponent2D* comp = m_rendering.Get();
		comp->m_object = this;
	}*/

	/*inline void SetType(const TYPE& type)
	{
		m_type = type;
	}*/

	// return null if object has one component has same type
	template <typename Comp>
	GameObject2D* AddComponent(const Handle<Comp>& component)
	{
		if constexpr (std::is_base_of_v<SubSystemComponent2D, Comp>)
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
		if (AddComponent(comp))
		{
			return comp;
		}
		return nullptr;
	}

	template <typename Comp>
	Handle<Comp> GetComponent() const
	{
		if constexpr (std::is_base_of_v<SubSystemComponent2D, Comp>)
		{
			return DynamicCast<Comp>(m_subSystemComponents[Comp::COMPONENT_ID]);
		}
		else
		{
			ComponentDtor dtor = GetDtor<Comp>();
			auto it = FindComponentFromDtor(dtor);

			if (it != m_components.end())
			{
				return DynamicCast<Comp>(it->ptr);
			}
			return nullptr;
		}
	}

	template <typename Comp>
	Comp* GetComponentRaw() const
	{
		if constexpr (std::is_base_of_v<SubSystemComponent2D, Comp>)
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
	template <typename Func>
	inline static void PostTraversal(GameObject2D* obj, Func func)
	{
		for (auto& child : obj->m_children)
		{
			PostTraversal(child.Get(), func);
		}

		func(obj);
	}

private:
	// transform includes obj transform
	template <typename Func>
	inline static void PostTraversalRoundedPositionTransformImpl(const Transform2D& transform, GameObject2D* obj, Func&& func)
	{
		Transform2D temp;
		for (auto& child : obj->m_children)
		{
			//child->Transform().Translation().Round();
			Transform2D::Joint(transform, child->Transform(), temp);
			PostTraversalRoundedPositionTransformImpl(temp, child.Get(), func);
		}

		func(obj, transform);
	}

public:
	template <typename Func>
	inline static void PostTraversalRoundedPositionTransform(GameObject2D* obj, Func&& func)
	{
		//obj->Transform().Translation().Round();
		PostTraversalRoundedPositionTransformImpl(obj->Transform(), obj, func);
	}

public:
	inline auto AddChild(const Handle<GameObject2D>& obj)
	{
		assert(obj->IsRoot());
		m_children.Push(obj);
		obj->m_parent = this;
	}

	inline auto& Child(size_t index)
	{
		return m_children[index];
	}

	inline auto& Children()
	{
		return m_children;
	}

	inline bool IsRoot()
	{
		return m_parent.IsNull();
	}

	inline const auto& GetAABB() const
	{
		return m_aabb;
	}

	inline auto GetScene() const
	{
		return m_scene;
	}

	inline GameObject2D* GetRoot()
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

	inline Transform2D& Transform()
	{
		return m_transform;
	}

	inline const Transform2D& GlobalTransform() const
	{
		return m_globalTransform;
	}

	inline Vec2& Scale()
	{
		return m_transform.Scale();
	}

	inline Vec2& Position()
	{
		return m_transform.Translation();
	}

	inline float& Rotation()
	{
		return m_transform.Rotation();
	}

	inline const auto& GetCachedTransform() const
	{
		return m_cachedTransform;
	}
	
};

NAMESPACE_END