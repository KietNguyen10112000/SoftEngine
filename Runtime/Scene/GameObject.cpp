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

void GameObject::RecalculateUpToDateTransform(ID parentIdx)
{
	auto readIdx = IdxTransformUpToDate();

	//auto idx = ReadTransformIdx();

	auto& readLocalTransform = m_localTransform[readIdx];
	auto& writeGlobalMat = m_globalTransformMat[WriteTransformIdx()];
	auto& writeLocalMat = m_localTransformMat[WriteTransformIdx()];
	WriteLocalTransform() = readLocalTransform;

	auto numTransformContributors = m_numTransformContributors.load(std::memory_order_relaxed);
	m_numTransformContributors.store(0, std::memory_order_relaxed);

	auto& contributors = m_transformContributors;
	for (uint32_t i = 0; i < numTransformContributors; i++)
	{
		contributors[i].func(this, contributors[i].comp);
	}

	writeLocalMat = readLocalTransform.ToTransformMatrix();
	auto parent = ParentUpToDate().Get();
	writeGlobalMat = writeLocalMat * (parent ? parent->m_globalTransformMat[parentIdx == INVALID_ID ? parent->ReadTransformIdx() : parentIdx] : Mat4::Identity());

	auto& children = m_lastChangeTreeIterationCount == 0 ? ReadChildren() : WriteChildren();
	for (auto& child : children)
	{
		child->RecalculateUpToDateTransform(WriteTransformIdx());
	}
}

void GameObject::IndirectSetLocalTransform(const Transform& transform)
{
	if (transform.Equals(ReadLocalTransform())) return;

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
	m_scene->OnObjectTransformChanged(obj);

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

void GameObject::Serialize(Serializer* serializer)
{
}

void GameObject::Deserialize(Serializer* serializer)
{
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

Handle<Serializable> GameObject::Clone(Serializer* serializer)
{
	//assert(!IsInAnyScene());

	auto& addresses = serializer->GetAddressMap();

	Handle<GameObject> ret = mheap::New<GameObject>();

	addresses.insert({ this, ret.Get() });

	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& comp = m_mainComponents[i];
		if (comp)
		{
			auto newComp = comp->Clone(serializer);
			if (newComp)
			{
				ret->m_mainComponents[i] = StaticCast<MainComponent>(newComp);
				ret->m_mainComponents[i]->m_object = ret;
				((HasMainComponentState*)ret->m_hasMainComponent.UpToDateRead())->hasComponents[i] = true;
			}
		}
	}

	auto& children = ReadChildren();
	for (size_t i = 0; i < children.size(); i++)
	{
		auto child = StaticCast<GameObject>(children[i]->Clone(serializer));
		ret->AddChild(child);
	}

	for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; i++)
	{
		ret->m_localTransform[i] = m_localTransform[i];
		ret->m_globalTransformMat[i] = m_globalTransformMat[i];
		ret->m_localTransformMat[i] = m_localTransformMat[i];
	}

	ret->Name() = Name();

	return ret;
}

NAMESPACE_END