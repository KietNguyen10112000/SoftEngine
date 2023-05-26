#include "Script2D.h"

#include "SubSystems2D/Script/ScriptSystem2D.h"

NAMESPACE_BEGIN

void Script2D::OnComponentAddedToScene()
{
	auto scene = m_object->GetScene();
	m_scene = scene;

	m_physicsInterface.m_scriptSystem = m_scene->GetScriptSystem();

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

void Script2D::OnStart() {};
void Script2D::OnUpdate(float dt) {};
void Script2D::OnCollide(GameObject2D* another, const Collision2DPair& pair) 
{
	
};

void Script2D::OnCollisionEnter(GameObject2D* another, const Collision2DPair& pair) 
{
};

void Script2D::OnCollisionExit(GameObject2D* another, const Collision2DPair& pair) 
{
};

void Script2D::OnGUI() {};

NAMESPACE_END