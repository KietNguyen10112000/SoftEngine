#include "AnimatorEditorSaveData.h"

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
	j["EditorName"] = m_name;
}

void AnimatorEditorSaveData::DeserializeFromJson(Serializer* serializer, const json& j)
{
	m_objectUUID = j["ObjectUUID"];
	m_name = j["EditorName"];
}

Handle<ClassMetadata> AnimatorEditorSaveData::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimatorEditorSaveData::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}
