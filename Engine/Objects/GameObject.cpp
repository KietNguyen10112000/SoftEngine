#include "GameObject.h"

#include "SubSystems/SubSystem.h"

#include "Objects/Scene/Scene.h"

NAMESPACE_BEGIN

void GameObject::InvokeAddRootComponentToSubSystem(SubSystemComponent* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->AddSubSystemComponent(comp, COMPONENT_ID);
}

void GameObject::InvokeRemoveRootComponentFromSubSystem(SubSystemComponent* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->RemoveSubSystemComponent(comp, COMPONENT_ID);
}

NAMESPACE_END