#include "AnimatorEditorSaveData.h"

#include "AnimatorEditorTab.h"

AnimatorEditorSaveData::AnimatorEditorSaveData(AnimatorEditorTab* tab) : m_tab(tab)
{
}

void AnimatorEditorSaveData::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void AnimatorEditorSaveData::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimatorEditorSaveData::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimatorEditorSaveData::SerializeToJson(Serializer* serializer, json& j) const
{
	j["ObjectUUID"] = m_objectUUID;
	j["SceneUUID"] = m_sceneUUID;
	j["CameraUUID"] = m_cameraUUID;
	j["EditorName"] = m_name;

	m_tab->WriteNodeDataToJson(serializer, j);
}

void AnimatorEditorSaveData::DeserializeFromJson(Serializer* serializer, const json& j)
{
	m_objectUUID = j["ObjectUUID"];
	m_sceneUUID = j["SceneUUID"];
	m_cameraUUID = j["CameraUUID"];
	m_name = j["EditorName"];

	m_tab->ReadNodeDataFromJson(serializer, j);
}

Handle<ClassMetadata> AnimatorEditorSaveData::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimatorEditorSaveData::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}
