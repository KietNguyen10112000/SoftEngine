#include "Script.h"

#include "SubSystems/Script/ScriptSystem.h"

NAMESPACE_BEGIN

void Script::OnComponentAddedToScene(GameObject* obj)
{
	m_object = obj;

	auto scene = obj->GetScene();
	scene->GetScriptSystem()->AddScript(this);

	OnStart();
}

void Script::OnComponentAdded(GameObject* object)
{
}

void Script::OnComponentRemoved(GameObject* object)
{
}

void Script::OnComponentRemovedFromScene(GameObject* object)
{
}

void Script::SetAsMain(GameObject* object)
{
}

void Script::SetAsExtra(GameObject* object)
{
}

void Script::ResolveConflict(GameObject* object)
{
}

bool Script::IsConflict()
{
	return true;
}

NAMESPACE_END