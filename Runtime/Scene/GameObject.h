#pragma once

#include "Core/Memory/Memory.h"
#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "MODIFICATION_STATE.h"

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

	SERIALIZABLE_CLASS(GameObject);

	struct TransformContributor
	{
		void* comp;
		void (*func)(GameObject* object, void* comp);
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
	bool m_padd[2];
	Spinlock m_modificationLock;
	uint32_t m_modificationState = MODIFICATION_STATE::NONE;

	std::atomic<size_t> m_isRecoredChangeTransformIteration = { 0 };
	size_t m_updatedTransformIteration = 0;

	ID m_sceneId = INVALID_ID;

	String m_indexedName;
	String m_name;

	TransformContributor m_transformContributors[MainSystemInfo::COUNT] = {};
	std::atomic<uint32_t> m_numTransformContributors = { 0 };

	Mat4		m_globalTransformMat	[NUM_TRANSFORM_BUFFERS] = {};
	Mat4		m_localTransformMat		[NUM_TRANSFORM_BUFFERS] = {};
	Transform	m_localTransform		[NUM_TRANSFORM_BUFFERS] = {};
	uint32_t	m_transformReadIdx								= 0;

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
		auto& slot = m_mainComponents[Comp::COMPONENT_ID];
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

	void IndirectSetLocalTransform(const Transform& transform);

	inline void ContributeLocalTransform(void* p, void (*func)(GameObject*, void*))
	{
		auto idx = m_numTransformContributors++;
		assert(idx < MainSystemInfo::COUNT);
		m_transformContributors[idx].comp = p;
		m_transformContributors[idx].func = func;
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
		if (func(this)) return;
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
	virtual void Serialize(Serializer* serializer) override;
	virtual void Deserialize(Serializer* serializer) override;
	virtual void CleanUp() override {}
	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;
	virtual void OnPropertyChanged(const UnknownAddress& var) override;

public:
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

	inline auto GetScene()
	{
		return m_scene;
	}

	inline bool IsInAnyScene()
	{
		return m_scene && m_sceneId != INVALID_ID;
	}

	inline const auto& GetLocalTransform()
	{
		return ReadLocalTransform();
	}

	inline void SetLocalTransform(const Transform& transform)
	{
		if (!IsInAnyScene()) 
		{
			// no need multithread, direct update
			for (auto& trans : m_localTransform)
			{
				trans = transform;
			}
			RecalculateReadTransform();
			RecalculateWriteTransform();
			return;
		}
		
		IndirectSetLocalTransform(transform);
	}

};

NAMESPACE_END