#include "Script.h"

#include "SubSystems/Script/ScriptSystem.h"

NAMESPACE_BEGIN

void Script::OnComponentAddedToScene()
{
	auto scene = m_object->GetScene();

	m_scene = scene;

	scene->GetScriptSystem()->AddScript(this);

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

void Script::ResolveConflict()
{
}

bool Script::IsConflict()
{
	return true;
}

NAMESPACE_END