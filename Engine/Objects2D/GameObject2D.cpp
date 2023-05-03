#include "GameObject2D.h"

#include "SubSystems2D/SubSystem2D.h"

#include "Objects2D/Scene2D/Scene2D.h"

NAMESPACE_BEGIN

void GameObject2D::InvokeAddRootComponentToSubSystem(SubSystemComponent2D* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->AddSubSystemComponent(comp);
}

void GameObject2D::InvokeRemoveRootComponentFromSubSystem(SubSystemComponent2D* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->RemoveSubSystemComponent(comp);
}

NAMESPACE_END