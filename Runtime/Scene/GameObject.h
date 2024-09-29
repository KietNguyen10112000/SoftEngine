#pragma once

#include "Core/Memory/Memory.h"
#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "MODIFICATION_STATE.h"

#include "Runtime/Config.h"

#include "Scene.h"

NAMESPACE_BEGIN

class Scene;

class API GameObject final : public Serializable
{
public:
	using ComponentDtor = void(*)(void*);
	struct ComponentSlot
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

	// define how's local and global transform relative to
	struct TRANSFORM_CONSTRAINT
	{
		enum TYPE
		{
			LOCAL_TO_GLOBAL,
			GLOBAL_TO_LOCAL,
			FREE
		};
	};

	SERIALIZABLE_CLASS(GameObject);

private:
	MAIN_SYSTEM_FRIEND_CLASSES();
	friend class ModifiedRecorder;
	friend class GameObjectDependencies;

	struct ModifiedFlag
	{
		enum Flag
		{
			HEIRARCHY		= (1 << 0),
			COMPONENT		= (1 << 1),
			TRANSFORM		= (1 << 2)
		};
	};

	Handle<MainComponent> m_mainComponents[MainSystemInfo::COUNT] = {};
	Handle<MainComponent> m_committedComponents[MainSystemInfo::COUNT] = {};

	// external components
	Array<ComponentSlot> m_components = {};

	GameObject*					m_root = this;
	GameObject*					m_committedRoot = this;
	Handle<GameObject>			m_parent = nullptr;
	Array<Handle<GameObject>>	m_children = {};

	ID m_parentIdx = INVALID_ID;

	TRANSFORM_CONSTRAINT::TYPE m_transformConstraint = TRANSFORM_CONSTRAINT::LOCAL_TO_GLOBAL;

	Transform m_localTransform = {};
	Transform m_committedLocalTransform = {};

	Mat4 m_globalTransform = {};
	Mat4 m_committedGlobalTransform = {};

	uint32_t m_componentIdModifyTransform = INVALID_ID;
	uint32_t m_committedComponentIdModifyTransform = INVALID_ID;

	//size_t m_localTransformWriteCount = 0;
	//size_t m_localTransformWriteCount = 0;

	Scene* m_scene = nullptr;
	ID m_sceneId = INVALID_ID;

	Scene* m_committedScene = nullptr;
	ID m_committedSceneId = INVALID_ID;

	String m_name;
	size_t m_tag = INVALID_ID;

	bool m_isLongLife = true;
	bool m_recorded = false;
	bool m_forceRefreshTransform = false;
	Spinlock m_lock;

	uint32_t m_modifiedFlags = 0;

	int m_dependenciesRecordedValue = 0;
	byte m_padd[4];

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_mainComponents); 
		tracer->Trace(m_committedComponents);
		tracer->Trace(m_components);
		tracer->Trace(m_parent);
		tracer->Trace(m_children);
	}

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
	GameObject* AddMainComponentDirect(const Handle<Comp>& component)
	{
		m_lock.lock();

		auto& slot = m_mainComponents[Comp::COMPONENT_ID];

		assert(slot.Get() == nullptr);

		slot = component;
		component->m_object = this;
		//component->OnComponentAdded();

		m_lock.unlock();

		return this;
	}

	GameObject* AddMainComponentDefer(ID COMPONENT_ID, const Handle<MainComponent>& component);

	template <typename Comp>
	GameObject* RemoveMainComponentDirect(Comp* component)
	{
		m_lock.lock();

		auto& slot = m_mainComponents[Comp::COMPONENT_ID];

		assert(slot.Get() != nullptr);

		slot = nullptr;
		component->m_object = nullptr;

		m_lock.unlock();

		return this;
	}

	GameObject* RemoveMainComponentDefer(ID COMPONENT_ID, MainComponent* component);

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
		m_lock.lock();

		ComponentDtor dtor = GetDtor<Comp>();
		auto it = FindComponentFromDtor(dtor);

		if (it != m_components.end())
		{
			m_lock.unlock();
			return nullptr;
		}

		ComponentSlot slot = {};
		slot.identifier = (ID)dtor;
		slot.ptr = component;
		m_components.Push(slot);

		m_lock.unlock();

		return this;
	}

	template <typename Comp>
	GameObject* RemoveNormalComponent(Comp* component)
	{
		m_lock.lock();

		ComponentDtor dtor = GetDtor<Comp>();
		auto it = FindComponentFromDtor(dtor);

		if ((void*)component != it->ptr.Get())
		{
			m_lock.unlock();
			return nullptr;
		}

		if (it != m_components.end())
		{
			auto idx = it - m_components.data();
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK_BY_ID(m_components, idx);
		}

		m_lock.unlock();

		return this;
	}

	inline void Commit()
	{
		if (m_modifiedFlags & ModifiedFlag::COMPONENT)
		{
			for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
			{
				m_committedComponents[i] = m_mainComponents[i];
			}
		}
		
		if (m_modifiedFlags & ModifiedFlag::HEIRARCHY)
		{
			m_committedRoot = m_root;
			m_committedScene = m_scene;
			m_committedSceneId = m_sceneId;
		}

		if (m_modifiedFlags & ModifiedFlag::TRANSFORM)
		{
			m_committedLocalTransform = m_localTransform;
			m_committedGlobalTransform = m_globalTransform;
			m_committedComponentIdModifyTransform = m_componentIdModifyTransform;
			m_componentIdModifyTransform = INVALID_ID;
		}

		m_modifiedFlags = 0;
		m_forceRefreshTransform = false;
	}

	void RecalculateTransform(const Mat4& parentTransform);

	// recursive
	void RecordAllComponetsAsModified();

public:
	//~GameObject();

	// return null if object has one component has same type
	template <typename Comp>
	GameObject* AddComponent(const Handle<Comp>& component)
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			if (m_mainComponents[Comp::COMPONENT_ID].Get() != nullptr)
			{
				return nullptr;
			}

			{
				AddMainComponentDefer(Comp::COMPONENT_ID, component);
			}

			return AddMainComponentDirect(component);
		}
		else
		{
			return AddNormalComponent(component);
		}
	}

	// return this if object has component, null on ow
	template <typename Comp>
	inline GameObject* RemoveComponent(const Handle<Comp>& component)
	{
		return RemoveComponentRaw(component.Get());
	}

	// return this if object has component, null on ow
	template <typename Comp>
	inline GameObject* RemoveComponentRaw(Comp* component)
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			if (m_mainComponents[Comp::COMPONENT_ID].Get() == nullptr)
			{
				return nullptr;
			}

			if (component != nullptr && (void*)m_mainComponents[Comp::COMPONENT_ID].Get() != (void*)component)
			{
				return nullptr;
			}

			{
				MainComponent* _comp = component == nullptr ? m_mainComponents[Comp::COMPONENT_ID].Get() : component;
				RemoveMainComponentDefer(Comp::COMPONENT_ID, _comp);
			}

			return RemoveMainComponentDirect(component);
		}
		else
		{
			return RemoveNormalComponent(component);
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
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			if (m_mainComponents[Comp::COMPONENT_ID].Get() == nullptr)
			{
				return nullptr;
			}

			assert(dynamic_cast<Comp*>(m_mainComponents[Comp::COMPONENT_ID].Get()) != nullptr);
			return StaticCast<Comp>(m_mainComponents[Comp::COMPONENT_ID]);
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

	template <typename Comp, bool RESTRICTED = true>
	Comp* GetCommittedComponentRaw() const
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			if constexpr (!RESTRICTED)
			{
				if (m_committedComponents[Comp::COMPONENT_ID].Get())
				{
					return dynamic_cast<Comp*>(m_committedComponents[Comp::COMPONENT_ID].Get());
				}
			}
			else
			{
#ifdef _DEBUG
				if (m_committedComponents[Comp::COMPONENT_ID].Get())
					assert(dynamic_cast<Comp*>(m_committedComponents[Comp::COMPONENT_ID].Get()) != nullptr);
#endif // _DEBUG

				return (Comp*)(m_committedComponents[Comp::COMPONENT_ID].Get());
			}
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

	template <typename Comp>
	Comp* GetComponentRaw() const
	{
		return GetCommittedComponentRaw<Comp>();
	}

	// consistency chec
	template <typename Comp>
	inline bool HasComponent()
	{
		return GetCommittedComponentRaw<Comp, false>() != nullptr;
	}

	// consistency check
	inline bool HasComponent(ID COMPONENT_ID)
	{
		return m_committedComponents[COMPONENT_ID].Get() != nullptr;
	}

private:
	void _AddChild(const Handle<GameObject>& obj, ID index);
	void _RemoveFromParent(bool keepChildrenOrder);

public:
	void AddChild(const Handle<GameObject>& obj, ID index = INVALID_ID);
	void RemoveFromParent(bool keepChildrenOrder = false);

	// if this object is in scene => remove from scene, else remove from parent
	void RemoveSelf(bool keepChildrenOrder = false);

	template <typename Func>
	void ForEachChildren(Func func)
	{
		for (auto& child : m_children)
		{
			func(child.Get());
		}
	}

	template <typename Func>
	void PreTraversal(Func func)
	{
		if (func(this)) return;
		m_lock.lock();
		for (auto& child : m_children)
		{
			child->PreTraversal(func);
		}
		m_lock.unlock();
	}

	template <typename Func>
	void PreTraversal1(Func func)
	{
		func(this);
		m_lock.lock();
		for (auto& child : m_children)
		{
			child->PreTraversal1(func);
		}
		m_lock.unlock();
	}

	template <typename Func>
	void PostTraversal(Func func)
	{
		m_lock.lock();
		for (auto& child : m_children)
		{
			child->PostTraversal(func);
		}
		m_lock.unlock();
		func(this);
	}

protected:
	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

public:
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	inline const auto& Parent() const
	{
		return m_parent;
	}

	inline const auto& ParentIdx() const
	{
		return m_parentIdx;
	}

	inline const auto& Children() const
	{
		return m_children;
	}

	inline auto GetRoot()
	{
		return m_root;
	}

	inline auto GetCommittedRoot()
	{
		return m_committedRoot;
	}

	inline auto& Name()
	{
		return m_name;
	}

	inline const auto& Tag() const
	{
		return m_tag;
	}

	inline auto* Lock()
	{
		return &m_lock;
	}

	inline auto GetCommittedScene()
	{
		return m_committedScene;
	}

	inline auto GetScene()
	{
		return GetCommittedScene();
	}

	inline auto GetCurrentScene()
	{
		return m_scene;
	}

	inline bool IsInAnyScene()
	{
		return GetCommittedScene() != nullptr;
	}

	inline const auto& GetLocalTransform()
	{
		return m_localTransform;
	}

	inline const auto& GetCommittedLocalTransform()
	{
		return m_committedLocalTransform;
	}

	inline auto& GetCommittedGlobalTransform() const
	{
		return m_committedGlobalTransform;
	}

	void SetLocalTransform(const Transform& transform,  ID SRC_COMPONENT_ID = INVALID_ID - 1);
	void SetGlobalTransform(const Mat4& transform, ID SRC_COMPONENT_ID = INVALID_ID - 1, 
		TRANSFORM_CONSTRAINT::TYPE transformConstraint = TRANSFORM_CONSTRAINT::FREE, bool ignoreSameTransform = false);

	void CopyTransform(GameObject* obj);

	void ForceRefreshTransform(ID SRC_COMPONENT_ID = INVALID_ID - 1, bool recursive = false);

};

NAMESPACE_END