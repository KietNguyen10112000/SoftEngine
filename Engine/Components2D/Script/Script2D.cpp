#include "Script2D.h"

#include "SubSystems2D/Script/ScriptSystem2D.h"

NAMESPACE_BEGIN

void Script2D::OnComponentAddedToScene()
{
	auto scene = m_object->GetScene();
	m_scene = scene;

	OnStart();
}

void Script2D::OnComponentAdded()
{
}

void Script2D::OnComponentRemoved()
{
}

void Script2D::OnComponentRemovedFromScene()
{
}

void Script2D::SetAsMain()
{
}

void Script2D::SetAsExtra()
{
}

void Script2D::ResolveBranch()
{
}

bool Script2D::IsNewBranch()
{
	return true;
}

NAMESPACE_END