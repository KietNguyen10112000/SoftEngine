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
	// one for read, one for write, then swap between them
	constexpr static size_t NUM_TRANSFORM_BUFFERS = 3;

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
			RESTRICTED,
			FREE
		};
	};

	SERIALIZABLE_CLASS(GameObject);

private:
	MAIN_SYSTEM_FRIEND_CLASSES();

	Handle<MainComponent> m_mainComponents[MainSystemInfo::COUNT] = {};
	Handle<MainComponent> m_committedComponents[MainSystemInfo::COUNT] = {};

	// external components
	Array<ComponentSlot> m_components = {};

	Handle<GameObject>			m_parent = nullptr;
	Array<Handle<GameObject>>	m_children = {};

	Transform m_localTransform = {};
	Mat4 m_globalTransform = {};

	Mat4 m_committedGlobalTransform = {};

	//size_t m_localTransformWriteCount = 0;
	//size_t m_localTransformWriteCount = 0;

	Scene* m_scene = nullptr;
	ID m_sceneId = INVALID_ID;

	String m_name;

	bool m_isLongLife = true;
	Spinlock m_lock;
	bool m_padd2[2];

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
		m_modificationLock.lock();

		auto& slot = m_mainComponents[Comp::COMPONENT_ID];

		assert(slot.Get() == nullptr);

		slot = component;
		component->m_object = this;
		//component->OnComponentAdded();

		m_modificationLock.unlock();

		return this;
	}

	GameObject* AddMainComponentDefer(ID COMPONENT_ID, const Handle<MainComponent>& component);

	template <typename Comp>
	GameObject* RemoveMainComponentDirect(Comp* component)
	{
		m_modificationLock.lock();

		auto& slot = m_mainComponents[Comp::COMPONENT_ID];

		assert(slot.Get() != nullptr);

		slot = nullptr;
		component->m_object = nullptr;

		m_modificationLock.unlock();

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
		m_modificationLock.lock();

		ComponentDtor dtor = GetDtor<Comp>();
		auto it = FindComponentFromDtor(dtor);

		if (it != m_components.end())
		{
			m_modificationLock.unlock();
			return nullptr;
		}

		ComponentSlot slot = {};
		slot.identifier = (ID)dtor;
		slot.ptr = component;
		m_components.Push(slot);

		m_modificationLock.unlock();

		return this;
	}

	template <typename Comp>
	GameObject* RemoveNormalComponent(Comp* component)
	{
		m_modificationLock.lock();

		ComponentDtor dtor = GetDtor<Comp>();
		auto it = FindComponentFromDtor(dtor);

		if ((void*)component != it->ptr.Get())
		{
			m_modificationLock.unlock();
			return nullptr;
		}

		if (it != m_components.end())
		{
			auto idx = it - m_components.data();
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK_BY_ID(m_components, idx);
		}

		m_modificationLock.unlock();

		return this;
	}
	
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

			if (IsInAnyScene())
			{
				return AddMainComponentDefer(Comp::COMPONENT_ID, component);
			}

			((HasMainComponentState*)m_hasMainComponent.UpToDateRead())->hasComponents[Comp::COMPONENT_ID] = true;
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

			if (IsInAnyScene())
			{
				MainComponent* _comp = component == nullptr ? m_mainComponents[Comp::COMPONENT_ID].Get() : component;
				return RemoveMainComponentDefer(Comp::COMPONENT_ID, _comp);
			}

			((HasMainComponentState*)m_hasMainComponent.UpToDateRead())->hasComponents[Comp::COMPONENT_ID] = false;
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

	template <typename Comp>
	Comp* GetCommittedComponentRaw() const
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
#ifdef _DEBUG
			if (m_committedComponents[Comp::COMPONENT_ID].Get())
				assert(dynamic_cast<Comp*>(m_committedComponents[Comp::COMPONENT_ID].Get()) != nullptr);
#endif // _DEBUG

			return (Comp*)(m_committedComponents[Comp::COMPONENT_ID].Get());
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

	// consistency check
	template <typename Comp>
	inline bool HasComponent()
	{
		return GetComponentRaw<Comp>() != nullptr;
	}

	// consistency check
	inline bool HasComponent(ID COMPONENT_ID)
	{
		return m_mainComponents[COMPONENT_ID].Get() != nullptr;
	}

public:
	void AddChild(const Handle<GameObject>& obj);
	void RemoveFromParent();

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
		for (auto& child : m_children)
		{
			child->PreTraversal(func);
		}
	}

	template <typename Func>
	void PreTraversal1(Func func)
	{
		func(this);
		for (auto& child : m_children)
		{
			child->PreTraversal1(func);
		}
	}

	template <typename Func>
	void PostTraversal(Func func)
	{
		for (auto& child : m_children)
		{
			child->PostTraversal(func);
		}
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

	inline const auto& Children() const
	{
		return m_children;
	}

	inline auto& Name()
	{
		return m_name;
	}

	inline auto GetScene()
	{
		return m_scene;
	}

	inline bool IsInAnyScene()
	{
		return m_scene != nullptr;
	}

	inline const auto& GetLocalTransform()
	{
		return m_localTransform;
	}

	inline auto& GetCommittedGlobalTransform() const
	{
		return m_committedGlobalTransform;
	}

	void SetLocalTransform(const Transform& transform,  ID SRC_COMPONENT_ID = INVALID_ID - 1, TRANSFORM_CONSTRAINT::TYPE constranint = TRANSFORM_CONSTRAINT::RESTRICTED);
	void SetGlobalTransform(const Transform& transform, ID SRC_COMPONENT_ID = INVALID_ID - 1, TRANSFORM_CONSTRAINT::TYPE constranint = TRANSFORM_CONSTRAINT::FREE);

};

NAMESPACE_END