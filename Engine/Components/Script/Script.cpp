#include "Script.h"

#include "SubSystems/Script/ScriptSystem.h"

NAMESPACE_BEGIN

void Script::OnComponentAddedToScene()
{
	auto scene = m_object->GetScene();
	m_scene = scene;
	OnStart();
}

void Script::OnComponentAdded()
{
}

void Script::OnComponentRemoved()
{
}

void Script::OnComponentRemovedFromScene()
{
}

void Script::SetAsMain()
{
}

void Script::SetAsExtra()
{
}

void Script::ResolveBranch()
{
}

bool Script::IsNewBranch()
{
	return true;
}

NAMESPACE_END