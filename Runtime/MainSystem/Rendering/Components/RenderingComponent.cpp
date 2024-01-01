#include "RenderingComponent.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

void RenderingComponent::OnTransformChanged()
{
	m_globalTransform = GetGameObject()->ReadGlobalTransformMat();
	//m_globalTransform = GetGameObject()->ReadGlobalTransform().ToTransformMatrix();
}

NAMESPACE_END