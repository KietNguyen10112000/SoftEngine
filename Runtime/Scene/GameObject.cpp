#include "GameObject.h"

#include "Scene.h"

NAMESPACE_BEGIN

GameObject* GameObject::AddMainComponentDefer(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	// [TODO]: will implement
	//assert(0);
	component->m_object = this;

	m_scene->BeginWrite(m_hasMainComponent);
	m_hasMainComponent.Write()->hasComponents[COMPONENT_ID] = true;
	m_scene->EndWrite(m_hasMainComponent);

	m_scene->AddComponent(COMPONENT_ID, component);
	return this;
}

GameObject* GameObject::RemoveMainComponentDefer(ID COMPONENT_ID, MainComponent* component)
{
	// [TODO]: will implement
	//assert(0);

	m_scene->BeginWrite(m_hasMainComponent);
	m_hasMainComponent.Write()->hasComponents[COMPONENT_ID] = false;
	m_scene->EndWrite(m_hasMainComponent);

	m_scene->RemoveComponent(COMPONENT_ID, component);
	return this;
}

void GameObject::RecalculateReadLocalTransform()
{
	auto* read = (Transform*)&ReadLocalTransform();
	auto& globalMat = ReadGlobalTransformMat();

	Mat4 localTransformMat;
	if (Parent().IsNull()) 
	{
		localTransformMat = globalMat;
	}
	else
	{
		auto& globalParentMat = Parent()->ReadGlobalTransformMat();
		localTransformMat = globalMat * globalParentMat.GetInverse();
	}

	localTransformMat.Decompose(read->Scale(), read->Rotation(), read->Translation());
}

void GameObject::RecalculateTransform(size_t idx)
{
	auto& localMat = m_localTransformMat[idx];
	auto& globalMat = m_globalTransformMat[idx];
	localMat = m_localTransform[idx].ToTransformMatrix();
	globalMat = localMat * (ParentUpToDate().Get() ? ParentUpToDate()->m_globalTransformMat[idx] : Mat4::Identity());

	auto& children = m_lastChangeTreeIterationCount == 0 ? ReadChildren() : WriteChildren();
	for (auto& child : children)
	{
		child->RecalculateTransform(idx);
	}
}

void GameObject::RecalculateUpToDateTransformBegin(ID parentIdx)
{
	auto readIdx = IdxTransformUpToDate();

	//auto idx = ReadTransformIdx();

	auto& readLocalTransform = m_localTransform[readIdx];
	auto& writeGlobalMat = m_globalTransformMat[WriteTransformIdx()];

	auto& writeLocalMat = m_localTransformMat[WriteTransformIdx()];
	m_updatedTransformIteration = m_scene->GetIterationCount();
	WriteLocalTransform() = readLocalTransform;

	writeLocalMat = readLocalTransform.ToTransformMatrix();
	auto parent = ParentUpToDate().Get();

	auto parentTransform = (parent ? parent->m_globalTransformMat[parentIdx == INVALID_ID ? parent->ReadTransformIdx() : parentIdx] : Mat4::Identity());
	writeGlobalMat = writeLocalMat * parentTransform;

	auto numTransformContributors = m_numTransformContributors.load(std::memory_order_relaxed);
	m_numTransformContributors.store(0, std::memory_order_relaxed);
	if (numTransformContributors != 0)
	{
		auto& _readLocalTransform = m_localTransform[WriteTransformIdx()];
		if (readIdx != WriteTransformIdx())
		{
			_readLocalTransform = m_localTransform[ReadTransformIdx()];
		}

		auto& contributors = m_transformContributors;
		for (uint32_t i = 0; i < numTransformContributors; i++)
		{
			contributors[i].func(this, _readLocalTransform, writeGlobalMat, contributors[i].comp);
		}
	}

	auto& children = m_lastChangeTreeIterationCount == 0 ? ReadChildren() : WriteChildren();
	for (auto& child : children)
	{
		child->RecalculateUpToDateTransform(WriteTransformIdx());
	}
}

void GameObject::RecalculateUpToDateTransform(ID parentIdx)
{
	auto readIdx = IdxTransformUpToDate();

	//auto idx = ReadTransformIdx();

	auto& readLocalTransform = m_localTransform[readIdx];
	auto& writeGlobalMat = m_globalTransformMat[WriteTransformIdx()];

	auto& writeLocalMat = m_localTransformMat[WriteTransformIdx()];
	m_updatedTransformIteration = m_scene->GetIterationCount();
	WriteLocalTransform() = readLocalTransform;

	writeLocalMat = readLocalTransform.ToTransformMatrix();
	auto parent = ParentUpToDate().Get();
	auto& parentTransform = parent->m_globalTransformMat[parentIdx];
	//writeGlobalMat = writeLocalMat * (parent ? parent->m_globalTransformMat[parentIdx == INVALID_ID ? parent->ReadTransformIdx() : parentIdx] : Mat4::Identity());

	writeGlobalMat = writeLocalMat * parentTransform;

	auto numTransformContributors = m_numTransformContributors.load(std::memory_order_relaxed);
	m_numTransformContributors.store(0, std::memory_order_relaxed);
	if (numTransformContributors != 0)
	{
		auto& _readLocalTransform = m_localTransform[WriteTransformIdx()];
		if (readIdx != WriteTransformIdx())
		{
			_readLocalTransform = m_localTransform[ReadTransformIdx()];
		}

		auto& contributors = m_transformContributors;
		for (uint32_t i = 0; i < numTransformContributors; i++)
		{
			contributors[i].func(this, _readLocalTransform, writeGlobalMat, contributors[i].comp);
		}
	}

	auto& children = m_lastChangeTreeIterationCount == 0 ? ReadChildren() : WriteChildren();
	for (auto& child : children)
	{
		child->RecalculateUpToDateTransform(WriteTransformIdx());
	}
}

void GameObject::IndirectSetLocalTransform(const Transform& transform)
{
	if (transform.Equals(ReadLocalTransform())) return;

	m_updatedTransformIteration = m_scene->GetIterationCount();
	m_lastWriteLocalTransformIterationCount = m_updatedTransformIteration;
	WriteLocalTransform() = transform;
	m_scene->OnObjectTransformChanged(this);
}

void GameObject::DuplicateTreeBuffer()
{
	WriteParent() = ReadParent();

	auto& oldChildren = ReadChildren();
	auto& newChildren = WriteChildren();

	assert(newChildren.size() <= oldChildren.size());

	uint32_t size = oldChildren.size();
	newChildren.Resize(size);
	for (uint32_t i = m_childCopyIdx; i < size; i++)
	{
		newChildren[i] = oldChildren[i];
	}
}

//GameObject::~GameObject()
//{
//	std::cout << "GameObject::~GameObject()\n";
//}

void GameObject::RemoveFromParent()
{
	assert(ParentUpToDate().Get() != nullptr);

	auto root = this;
	PostTraversalUpToDate(
		[root](GameObject* cur)
		{
			cur->m_root = root;
		}
	);

	if (!IsInAnyScene())
	{
		ReadParent() = nullptr;
		
		auto& list = ParentUpToDate()->ReadChildren();
		if constexpr (!Config::ORDERED_CHILD)
		{
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK(list, this, m_childInParentIdx);
		}
		else
		{
			list.Remove(&list[m_childInParentIdx]);
			for (uint32_t i = m_childInParentIdx; i < list.size(); i++)
			{
				list[i]->m_childInParentIdx--;
			}
		}

		m_childInParentIdx = INVALID_ID;

		return;
	}

	auto& parent = ParentUpToDate();
	m_treeLock.lock();
	parent->m_treeLock.lock();
	parent->m_modificationLock.lock();

	m_scene->DoRemoveFromParent(parent.Get(), this);
	//m_scene->OnObjectTransformChanged(obj);

	if (m_lastChangeTreeIterationCount != m_scene->GetIterationCount())
	{
		DuplicateTreeBuffer();
		m_lastChangeTreeIterationCount = m_scene->GetIterationCount();
	}

	if (parent->m_lastChangeTreeIterationCount != m_scene->GetIterationCount())
	{
		parent->DuplicateTreeBuffer();
		parent->m_lastChangeTreeIterationCount = m_scene->GetIterationCount();
	}

	WriteParent() = nullptr;
	auto& list = parent->WriteChildren();

	if constexpr (!Config::ORDERED_CHILD)
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(list, this, m_childInParentIdx);
	}
	else
	{
		list.Remove(&list[m_childInParentIdx]);
		for (uint32_t i = m_childInParentIdx; i < list.size(); i++)
		{
			list[i]->m_childInParentIdx--;
		}
	}

	uint32_t newCopyIdx = m_childInParentIdx;
	m_childCopyIdx = std::min(m_childCopyIdx, newCopyIdx);

	m_childInParentIdx = INVALID_ID;

	parent->m_modificationLock.unlock();
	parent->m_treeLock.unlock();
	m_treeLock.unlock();
}

void GameObject::AddChild(const Handle<GameObject>& obj)
{
	assert(obj->ParentUpToDate().Get() == nullptr);

	auto root = m_root;
	assert(root->ParentUpToDate().Get() == nullptr);
	obj->PostTraversalUpToDate(
		[root](GameObject* cur)
		{
			cur->m_root = root;
		}
	);

	if (!IsInAnyScene())
	{
		obj->ReadParent() = this;
		//obj->m_treeIdx = 1;
		//obj->m_childCopyIdx = 0;
		obj->m_childInParentIdx = ReadChildren().size();

		ReadChildren().Push(obj);
		//m_treeIdx = 1;
		//m_childCopyIdx = 0;

		RecalculateTransform(0);
		RecalculateTransform(1);

		return;
	}

	m_treeLock.lock();
	obj->m_treeLock.lock();
	obj->m_modificationLock.lock();

	m_scene->DoAddToParent(this, obj);

	m_updatedTransformIteration = m_scene->GetIterationCount();
	WriteLocalTransform() = ReadLocalTransform();
	m_scene->OnObjectTransformChanged(this);

	if (m_lastChangeTreeIterationCount != m_scene->GetIterationCount())
	{
		DuplicateTreeBuffer();
		m_lastChangeTreeIterationCount = m_scene->GetIterationCount();
	}

	if (obj->m_lastChangeTreeIterationCount != m_scene->GetIterationCount())
	{
		obj->DuplicateTreeBuffer();
		obj->m_lastChangeTreeIterationCount = m_scene->GetIterationCount();
	}

	obj->WriteParent() = this;
	obj->m_childInParentIdx = WriteChildren().size();
	WriteChildren().Push(obj);

	uint32_t newCopyIdx = ReadChildren().size();
	m_childCopyIdx = std::min(m_childCopyIdx, newCopyIdx);

	obj->m_modificationLock.unlock();
	obj->m_treeLock.unlock();
	m_treeLock.unlock();

	//assert(0);
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
			return Variant::Of(obj.ReadLocalTransform());
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
				((HasMainComponentState*)m_hasMainComponent.UpToDateRead())->hasComponents[i] = true;
			}
		}
	}

	auto& children = src->ReadChildren();
	for (size_t i = 0; i < children.size(); i++)
	{
		auto child = serializer->Clone(children[i]);
		AddChild(child);
	}

	for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; i++)
	{
		m_localTransform[i]		= src->m_localTransform[i];
		m_globalTransformMat[i] = src->m_globalTransformMat[i];
		m_localTransformMat[i]	= src->m_localTransformMat[i];
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
	j["LocalTransform"] = ReadLocalTransform();
	j["GlobalTransformMat"] = ReadGlobalTransformMat();

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
		auto& children = ((GameObject*)this)->ReadChildren();
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
	for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; i++)
	{
		m_localTransform[i] = localTransform;
		m_globalTransformMat[i] = globalTransform;
		m_localTransformMat[i] = localTransMat;
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