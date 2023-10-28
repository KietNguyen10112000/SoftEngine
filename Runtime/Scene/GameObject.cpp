#include "GameObject.h"

#include "Scene.h"

NAMESPACE_BEGIN

void GameObject::RecalculateTransform(size_t idx)
{
	auto& localMat = m_localTransformMat[idx];
	auto& globalMat = m_globalTransformMat[idx];
	localMat = m_localTransform[idx].ToTransformMatrix();
	globalMat = m_parent.Get() ? m_parent->m_globalTransformMat[idx] : Mat4::Identity() * localMat;

	for (auto& child : m_children)
	{
		child->RecalculateTransform(idx);
	}
}

void GameObject::IndirectSetLocalTransform(const Transform& transform)
{
	if (transform.Equals(ReadLocalTransform())) return;

	WriteLocalTransform() = transform;
	m_scene->OnObjectTransformChanged(this);
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

	metadata->AddProperty(Accessor::For("Local Transform", m_localTransform[0], this));

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

void GameObject::OnPropertyChanged(const UnknownAddress& var)
{
	if (var.Is(&m_localTransform[0]))
	{
		SetLocalTransform(m_localTransform[0]);
	}
}

NAMESPACE_END