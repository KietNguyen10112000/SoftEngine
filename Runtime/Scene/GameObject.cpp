#include "GameObject.h"

#include "Scene.h"

NAMESPACE_BEGIN

GameObject* GameObject::AddMainComponentDefer(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	// [TODO]: will implement
	//assert(0);
	component->m_object = this;
	m_scene->AddComponent(COMPONENT_ID, component);
	return this;
}

GameObject* GameObject::RemoveMainComponentDefer(ID COMPONENT_ID, MainComponent* component)
{
	// [TODO]: will implement
	//assert(0);
	m_scene->RemoveComponent(COMPONENT_ID, component);
	return this;
}

void GameObject::RecalculateTransform(size_t idx)
{
	auto& localMat = m_localTransformMat[idx];
	auto& globalMat = m_globalTransformMat[idx];
	localMat = m_localTransform[idx].ToTransformMatrix();
	globalMat = localMat * (ParentUpToDate().Get() ? ParentUpToDate()->m_globalTransformMat[idx] : Mat4::Identity());

	auto& children = Children();
	for (auto& child : children)
	{
		child->RecalculateTransform(idx);
	}
}

void GameObject::RecalculateUpToDateTransform(ID parentIdx)
{
	auto readIdx = (m_isRecoredChangeTransformIteration.load(std::memory_order_relaxed) == m_scene->GetIterationCount()) ? 
		WriteTransformIdx() : 
		ReadTransformIdx();

	//auto idx = ReadTransformIdx();

	auto& readLocalTransform = m_localTransform[readIdx];
	auto& writeGlobalMat = m_globalTransformMat[WriteTransformIdx()];
	auto& writeLocalMat = m_localTransformMat[WriteTransformIdx()];
	WriteLocalTransform() = readLocalTransform;

	writeLocalMat = readLocalTransform.ToTransformMatrix();
	writeGlobalMat = writeLocalMat * (ParentUpToDate().Get() ? ParentUpToDate()->m_globalTransformMat[parentIdx] : Mat4::Identity());

	auto& children = Children();
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
}

void GameObject::AddChild(const Handle<GameObject>& obj)
{
	assert(obj->Parent().Get() == nullptr);

	if (!IsInAnyScene())
	{
		obj->m_parent[0] = this;
		obj->m_treeIdx = 1;
		obj->m_childCopyIdx = 0;

		m_children[0].Push(obj);
		m_treeIdx = 1;
		m_childCopyIdx = 0;

		return;
	}

	m_treeLock.lock();
	obj->m_treeLock.lock();
	obj->m_modificationLock.lock();

	obj->m_modificationState = MODIFICATION_STATE::ADDING;

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

NAMESPACE_END