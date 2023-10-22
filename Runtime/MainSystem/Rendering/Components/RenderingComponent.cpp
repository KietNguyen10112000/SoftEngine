#include "RenderingComponent.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

void RenderingComponent::OnTransformChanged()
{
	m_globalTransform = GetGameObject()->ReadGlobalTransformMat();
}

NAMESPACE_END