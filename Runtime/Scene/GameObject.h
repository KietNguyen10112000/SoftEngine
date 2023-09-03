#pragma once

#include "Core/Memory/Memory.h"
#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

NAMESPACE_BEGIN

class Scene;

class GameObject final : Traceable<GameObject>, public Serializable
{
public:
	// one for read, one for write, then swap between them
	constexpr static size_t NUM_TRANSFORM_BUFFERS = 2;

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
	MAIN_SYSTEM_FRIEND_CLASSES();

	Handle<MainComponent> m_mainComponents[MainSystemInfo::COUNT] = {};

	// external components
	Array<ComponentSlot> m_components = {};

	Handle<GameObject> m_parent = nullptr;
	Array<Handle<GameObject>> m_children;

	Scene* m_scene = nullptr;
	bool m_isLongLife = true;
	bool m_isChangedTransform = false;
	bool m_padd[6];
	ID m_sceneId = INVALID_ID;

	String m_indexedName;
	String m_name;

	Mat4		m_globalTransformMat	[NUM_TRANSFORM_BUFFERS] = {};
	Mat4		m_localTransformMat		[NUM_TRANSFORM_BUFFERS] = {};
	Transform	m_localTransform		[NUM_TRANSFORM_BUFFERS] = {};
	size_t		m_transformReadIdx								= 0;

private:
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

	void RecalculateTransform(size_t idx);

	inline void RecalculateReadTransform()
	{
		RecalculateTransform(m_transformReadIdx);
	}

	inline void RecalculateWriteTransform()
	{
		RecalculateTransform((m_transformReadIdx + 1) % NUM_TRANSFORM_BUFFERS);
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

	template <typename Comp>
	Handle<Comp> GetComponent() const
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			assert(dynamic_cast<Comp>(m_mainComponents[Comp::COMPONENT_ID].Get()) != nullptr);
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
	Comp* GetComponentRaw() const
	{
		if constexpr (std::is_base_of_v<MainComponent, Comp>)
		{
			assert(dynamic_cast<Comp>(m_mainComponents[Comp::COMPONENT_ID].Get()) != nullptr);
			return (Comp*)(m_mainComponents[Comp::COMPONENT_ID].Get());
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
		func(this);
		for (auto& child : m_children)
		{
			child->PreTraversal(func);
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

public:
	virtual void Serialize(ByteStream& stream) override;
	virtual void Deserialize(ByteStreamRead& stream) override;
	virtual void CleanUp() override {}
	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;
	virtual void OnPropertyChanged(const UnknownAddress& var) override;

private:
	inline const Transform& ReadLocalTransform() const
	{
		return m_localTransform[m_transformReadIdx];
	}

	inline Transform& WriteLocalTransform()
	{
		return m_localTransform[(m_transformReadIdx + 1) % NUM_TRANSFORM_BUFFERS];
	}

	inline const Mat4& ReadGlobalTransformMat() const
	{
		return m_globalTransformMat[m_transformReadIdx];
	}

	inline Mat4& WriteGlobalTransformMat()
	{
		return m_globalTransformMat[(m_transformReadIdx + 1) % NUM_TRANSFORM_BUFFERS];
	}

	inline void UpdateTransformReadWrite()
	{
		m_transformReadIdx = (m_transformReadIdx + 1) % NUM_TRANSFORM_BUFFERS;
	}

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

	inline auto Scene()
	{
		return m_scene;
	}

	inline void SetTransform(const Transform& transform)
	{
		for (auto& trans : m_localTransform)
		{
			trans = transform;
		}
		RecalculateReadTransform();
		RecalculateWriteTransform();
	}

};

NAMESPACE_END