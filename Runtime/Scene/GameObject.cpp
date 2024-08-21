#include "GameObject.h"

#include "Scene.h"

#include "Runtime/Runtime.h"
#include "Scene/ModifiedRecorder.h"

#include "GameObjectDependenciesResolver.h"

NAMESPACE_BEGIN

GameObject* GameObject::AddMainComponentDefer(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	// [TODO]: will implement
	//assert(0);
	component->m_object = this;

	Runtime::Get()->GetModifiedRecorder()->RecordComponent(component, COMPONENT_ID);
	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::COMPONENT);
	return this;
}

GameObject* GameObject::RemoveMainComponentDefer(ID COMPONENT_ID, MainComponent* component)
{
	// [TODO]: will implement
	//assert(0);
	component->m_object = nullptr;

	Runtime::Get()->GetModifiedRecorder()->RecordComponent(component, COMPONENT_ID);
	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::COMPONENT);
	return this;
}

//GameObject::~GameObject()
//{
//	std::cout << "GameObject::~GameObject()\n";
//}

void GameObject::_RemoveFromParent(bool keepChildrenOrder)
{
	if (m_scene == 0 && m_parent == 0)
	{
		return;
	}

	if (m_parent == 0)
	{
		m_scene->RemoveObject(this);
		return;
	}

	m_parent->m_lock.lock();
	auto& arr = m_parent->m_children;
	if (!keepChildrenOrder)
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(arr, this, m_parentIdx);
	}
	else
	{
		arr.Remove(arr.begin() + m_parentIdx);
		for (size_t i = 0; i < arr.size(); i++)
		{
			arr[i]->m_parentIdx = i;
		}
	}
	m_parent->m_lock.unlock();

	m_parentIdx = INVALID_ID;
	m_parent = nullptr;

	RecalculateTransform(Mat4::Identity());

	auto root = this;
	m_root = root;
	{
		auto scene = m_scene;
		auto& recorder = Runtime::Get()->GetModifiedRecorder();
		PreTraversal1([&](GameObject* o)
			{
				o->m_scene = nullptr;
				o->m_root = root;
				for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
				{
					auto& comp = o->m_mainComponents[i];
					if (comp)
					{
						recorder->RecordComponent(comp, i);
					}
				}
			}
		);
	}

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::HEIRARCHY);
}

void GameObject::RemoveFromParent(bool keepChildrenOrder)
{
	GameObjectDependenciesRecorder recorder = m_scene;
	GameObjectDependencies::Get()->Collect(m_scene, this, &recorder);

	//bool removed = false;
	for (auto& o : recorder.GetRootObjects())
	{
		o->_RemoveFromParent(keepChildrenOrder);

		/*if (o == this)
		{
			removed = true;
		}*/
	}

	/*if (!removed)
	{
		_RemoveFromParent(keepChildrenOrder);
	}*/
}

void GameObject::RemoveSelf(bool keepChildrenOrder)
{
	if (m_parent == nullptr && m_scene)
	{
		m_scene->RemoveObject(this);
		return;
	}

	if (m_parent)
	{
		RemoveFromParent();
	}
}

void GameObject::RecalculateTransform(const Mat4& parentTransform)
{
	/*if (m_transformConstraint == TRANSFORM_CONSTRAINT::FREE)
	{
		return;
	}*/

	if (m_transformConstraint == TRANSFORM_CONSTRAINT::LOCAL_TO_GLOBAL)
	{
		m_globalTransform = m_localTransform.ToTransformMatrix() * parentTransform;
	}

	if (m_transformConstraint == TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL)
	{
		auto localMat = m_globalTransform * parentTransform.GetInverse();
		localMat.Decompose(m_localTransform.Scale(), m_localTransform.Rotation(), m_localTransform.Position());
	}

	if (m_globalTransform != m_committedGlobalTransform)
	{
		Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::TRANSFORM);
	}

	if (!IsInAnyScene())
	{
		m_committedGlobalTransform = m_globalTransform;
		m_committedLocalTransform = m_localTransform;
		for (auto& comp : m_mainComponents)
		{
			if (comp)
			{
				auto old = comp->m_committedObject;
				comp->m_committedObject = this;
				comp->OnTransformChanged();
				comp->m_committedObject = old;
			}
		}
	}

	m_lock.lock();
	for (auto& c : m_children)
	{
		c->RecalculateTransform(m_globalTransform);
	}
	m_lock.unlock();
}

void GameObject::RecordAllComponetsAsModified()
{
	auto& recorder = Runtime::Get()->GetModifiedRecorder();
	PreTraversal1([&](GameObject* o)
		{
			for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
			{
				auto& comp = o->m_mainComponents[i];
				if (comp)
				{
					recorder->RecordComponent(comp, i);
				}
			}
		}
	);
}

void GameObject::_AddChild(const Handle<GameObject>& obj, ID index)
{
	assert(obj->m_parent == nullptr); 
	assert(obj != this);

	obj->m_parent = this;

	m_lock.lock();

	if (index == INVALID_ID)
	{
		obj->m_parentIdx = m_children.size();
		m_children.Push(obj);
	}
	else
	{
		assert(index <= m_children.size());

		m_children.insert(m_children.begin() + index, obj);
		for (size_t i = 0; i < m_children.size(); i++)
		{
			auto& c = m_children[i];
			c->m_parentIdx = i;
		}
	}

	m_lock.unlock();

	obj->RecalculateTransform(m_globalTransform);

	{
		auto scene = m_scene;
		auto& recorder = Runtime::Get()->GetModifiedRecorder();
		obj->PreTraversal1([&](GameObject* o)
			{
				o->m_scene = scene;
				o->m_root = this->m_root;
				for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
				{
					auto& comp = o->m_mainComponents[i];
					if (comp)
					{
						recorder->RecordComponent(comp, i);
					}
				}
			}
		);
	}

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(obj, ModifiedFlag::HEIRARCHY);
}

void GameObject::AddChild(const Handle<GameObject>& obj, ID index)
{
	GameObjectDependenciesRecorder recorder = m_scene;
	GameObjectDependencies::Get()->Collect(m_scene, obj, &recorder);
	for (auto& o : recorder.GetRootObjects())
	{
		if (o != this)
		{
			_AddChild(o, index);
		}
	}
}

void GameObject::SetLocalTransform(const Transform& transform, ID SRC_COMPONENT_ID)
{
	if (m_localTransform == transform)
	{
		return;
	}

	m_modifiedFlags |= ModifiedFlag::TRANSFORM;
	m_componentIdModifyTransform = SRC_COMPONENT_ID;
	m_transformConstraint = TRANSFORM_CONSTRAINT::LOCAL_TO_GLOBAL;

	m_localTransform = transform;

	RecalculateTransform(m_parent ? m_parent->m_globalTransform : Mat4::Identity());
}

void GameObject::SetGlobalTransform(const Mat4& transform, ID SRC_COMPONENT_ID, TRANSFORM_CONSTRAINT::TYPE transformConstraint, bool ignoreSameTransform)
{
	if (!ignoreSameTransform && transform == m_globalTransform)
	{
		return;
	}

	m_modifiedFlags |= ModifiedFlag::TRANSFORM;
	m_componentIdModifyTransform = SRC_COMPONENT_ID;
	m_transformConstraint = transformConstraint;

	m_globalTransform = transform;

	//Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::TRANSFORM);

	////if (transformConstraint != TRANSFORM_CONSTRAINT::FREE)
	//{
	//	m_lock.lock();
	//	for (auto& c : m_children)
	//	{
	//		c->RecalculateTransform(m_globalTransform);
	//	}
	//	m_lock.unlock();
	//}

	RecalculateTransform(m_parent ? m_parent->m_globalTransform : Mat4::Identity());
}

void GameObject::CopyTransform(GameObject* obj)
{
	m_globalTransform = obj->m_globalTransform;
	m_committedGlobalTransform = obj->m_committedGlobalTransform;

	m_localTransform = obj->m_localTransform;
	m_committedLocalTransform = obj->m_committedLocalTransform;

	m_transformConstraint = obj->m_transformConstraint;
}

void GameObject::ForceRefreshTransform(ID SRC_COMPONENT_ID, bool recursive)
{
	m_forceRefreshTransform = true;
	m_componentIdModifyTransform = INVALID_ID;

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::TRANSFORM);

	if (recursive)
	{
		m_lock.lock();
		for (auto& c : m_children)
		{
			c->ForceRefreshTransform(SRC_COMPONENT_ID, recursive);
		}
		m_lock.unlock();
	}
}

Handle<ClassMetadata> GameObject::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("GameObject", this);

	auto accessor = Accessor(
		"Global Transform",
		1,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{

		},

		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto obj = (GameObject*)instance;
			return Variant::Of(Transform::FromTransformMatrix(obj->GetCommittedGlobalTransform()));
		},
		this
	);

	auto accessor2 = Accessor(
		"Local Transform",
		2,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{

		},

		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto obj = (GameObject*)instance;
			return Variant::Of(obj->GetLocalTransform());
		},
		this
	);

	metadata->AddProperty(accessor);
	metadata->AddProperty(accessor2);

	size_t i = 0;
	for (auto& comp : m_mainComponents)
	{
		if (comp)
		{
			auto compMeta = comp->GetMetadata(0);
			if (compMeta == nullptr)
			{
				compMeta = mheap::New<ClassMetadata>(comp->GetClassName(), comp.Get());
			}
			metadata->AddProperty(MainSystemInfo::COMPONENT_NAME[i], compMeta);
		}
		i++;
	}

	return metadata;
}

void GameObject::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	if (var.Is(1))
	{
		auto& global = newValue.As<Transform>().ToTransformMatrix();
		auto local = global * (Parent().Get() ? Parent()->GetCommittedGlobalTransform().GetInverse() : Mat4::Identity());
		SetLocalTransform(Transform::FromTransformMatrix(local));
	}

	if (var.Is(2))
	{
		SetLocalTransform(newValue.As<Transform>());
	}
}

void GameObject::CloneFrom(Serializer* serializer, Serializable* another)
{
	//assert(!IsInAnyScene());

	auto src = (GameObject*)another;

	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& comp = src->m_mainComponents[i];
		if (comp)
		{
			auto newComp = serializer->Clone(comp);
			if (newComp)
			{
				m_mainComponents[i] = newComp;
				m_mainComponents[i]->m_object = this;
			}
		}
	}

	auto& children = src->m_children;
	for (size_t i = 0; i < children.size(); i++)
	{
		auto child = serializer->Clone(children[i]);
		AddChild(child);
	}

	Name() = src->Name();
}

void GameObject::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void GameObject::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void GameObject::SerializeToJson(Serializer* serializer, json& j) const
{
	j["Name"] = ((GameObject*)this)->Name();
	j["LocalTransform"] = m_localTransform;
	j["GlobalTransformMat"] = m_globalTransform;
	j["TransformConstraint"] = m_transformConstraint;

	{
		auto arr = json::array();
		//arr.get_ref<json::array_t&>().resize(MainSystemInfo::COUNT);
		for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
		{
			auto& comp = m_mainComponents[i];
			if (!comp.IsNull())
			{
				arr.push_back(serializer->Serialize(comp));
			}
			else
			{
				arr.push_back(nullptr);
			}
		}
		j["MainComponents"] = arr;
	}

	{
		auto arr = json::array();
		auto& children = ((GameObject*)this)->m_children;
		for (size_t i = 0; i < children.size(); i++)
		{
			arr.push_back(serializer->Serialize(children[i]));
		}
		j["Children"] = arr;
	}
}

void GameObject::DeserializeFromJson(Serializer* serializer, const json& j)
{
	Name() = j["Name"];

	Transform localTransform = j["LocalTransform"];
	Mat4 globalTransform = j["GlobalTransformMat"];
	auto localTransMat = localTransform.ToTransformMatrix();
	m_localTransform = localTransform;
	m_globalTransform = globalTransform;

	if (j.contains("TransformConstraint"))
	{
		m_transformConstraint = j["TransformConstraint"];
	}

	{
		auto& arr = j["MainComponents"];
		//arr.get_ref<json::array_t&>().resize(MainSystemInfo::COUNT);
		for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
		{
			auto& j1 = arr[i];
			if (!j1.is_null())
			{
				serializer->Deserialize(j1, m_mainComponents[i]);
				m_mainComponents[i]->m_object = this;
				m_mainComponents[i]->m_committedObject = this;
			}
		}
	}

	{
		auto& arr = j["Children"];
		auto count = arr.size();
		Handle<GameObject> child;
		for (size_t i = 0; i < count; i++)
		{
			child = nullptr;
			serializer->Deserialize(arr[i], child);
			_AddChild(child, INVALID_ID);
		}
	}

	m_modifiedFlags = ModifiedFlag::COMPONENT | ModifiedFlag::TRANSFORM | ModifiedFlag::HEIRARCHY;
	Commit();
}

NAMESPACE_END