#include "Script.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

void Script::Serialize(Serializer* serializer)
{
}

void Script::Deserialize(Serializer* serializer)
{
}

void Script::CleanUp()
{
}

Handle<ClassMetadata> Script::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Script::OnPropertyChanged(const UnknownAddress& var)
{
}

//const char* Script::GetClassName()
//{
//	return nullptr;
//}

void Script::OnComponentAdded()
{
	m_scene = GetGameObject()->GetScene();
	OnStart();
}

void Script::OnComponentRemoved()
{
}

void Script::OnTransformChanged()
{
}

AABox Script::GetGlobalAABB()
{
	return AABox();
}

void Script::OnStart()
{
}

void Script::OnUpdate(float dt)
{
}

void Script::OnGUI()
{
}

NAMESPACE_END