#include "Script.h"

#include "Scene/GameObject.h"

#include "../ScriptingSystem.h"

NAMESPACE_BEGIN

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

void Script::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void Script::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void Script::SerializeToJson(Serializer* serializer, json& j) const
{
}

void Script::DeserializeFromJson(Serializer* serializer, const json& j)
{
}

void Script::CloneFrom(Serializer* serializer, Serializable* another)
{
}

Handle<ClassMetadata> Script::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Script::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END