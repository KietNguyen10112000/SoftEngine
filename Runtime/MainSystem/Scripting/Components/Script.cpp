#include "Script.h"

#include "Scene/GameObject.h"

#include "../ScriptingSystem.h"

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

void Script::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
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

void Script::FlushAsync()
{
	m_taskRunners[m_scene->GetPrevDeferBufferIdx()].Flush();
}

void Script::OnRecordAsync()
{
	m_scene->GetScriptingSystem()->OnScriptRecordAsyncTask(this);
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