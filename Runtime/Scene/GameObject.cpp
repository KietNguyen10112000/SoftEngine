#include "GameObject.h"

#include "Scene.h"

#include "Runtime/Runtime.h"
#include "Scene/ModifiedRecorder.h"

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

void GameObject::RemoveFromParent()
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

	auto& arr = m_parent->m_children;
	MANAGED_ARRAY_ROLL_TO_FILL_BLANK(arr, this, m_parentIdx);

	m_parentIdx = INVALID_ID;
	m_parent = nullptr;

	RecalculateTransform(Mat4::Identity());

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::HEIRARCHY);
}

void GameObject::RecalculateTransform(const Mat4& parentTransform)
{
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

	for (auto& c : m_children)
	{
		c->RecalculateTransform(m_globalTransform);
	}
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

void GameObject::AddChild(const Handle<GameObject>& obj)
{
	assert(obj->m_parent == nullptr);

	obj->m_parent = this;
	obj->m_parentIdx = m_children.size();
	m_children.Push(obj);

	obj->RecalculateTransform(m_globalTransform);

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(obj, ModifiedFlag::HEIRARCHY);
}

void GameObject::SetLocalTransform(const Transform& transform, ID SRC_COMPONENT_ID)
{
	if (m_localTransform == transform)
	{
		return;
	}

	m_modifiedFlags |= ModifiedFlag::TRANSFORM;
	m_componentIdModifyTransform = SRC_COMPONENT_ID;

	m_localTransform = transform;

	m_transformConstraint = TRANSFORM_CONSTRAINT::LOCAL_TO_GLOBAL;
	RecalculateTransform(m_parent ? m_parent->m_globalTransform : Mat4::Identity());
}

void GameObject::SetGlobalTransform(const Mat4& transform, ID SRC_COMPONENT_ID)
{
	if (transform == m_globalTransform)
	{
		return;
	}

	m_componentIdModifyTransform = SRC_COMPONENT_ID;

	m_globalTransform = transform;

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::TRANSFORM);

	for (auto& c : m_children)
	{
		c->RecalculateTransform(m_globalTransform);
	}

	m_transformConstraint = TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL;
}

void GameObject::ForceRefreshTransform(ID SRC_COMPONENT_ID)
{
	m_forceRefreshTransform = true;
	m_componentIdModifyTransform = INVALID_ID;

	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(this, ModifiedFlag::TRANSFORM);
}

Handle<ClassMetadata> GameObject::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("GameObject", this);

	auto accessor = Accessor(
		"Local Transform",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{

		},

		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto& obj = var.As<GameObject>();
			return Variant::Of(obj.GetLocalTransform());
		},
		this
	);

	metadata->AddProperty(accessor);

	size_t i = 0;
	for (auto& comp : m_mainComponents)
	{
		if (comp)
		{
			metadata->AddProperty(MainSystemInfo::COMPONENT_NAME[i], comp->GetMetadata(sign));
		}
		i++;
	}

	return metadata;
}

void GameObject::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	if (var.Is(this))
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
			AddChild(child);
		}
	}
}

NAMESPACE_END