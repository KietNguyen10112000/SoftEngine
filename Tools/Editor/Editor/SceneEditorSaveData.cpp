#include "SceneEditorSaveData.h"

#include "SceneEditorTab.h"

SceneEditorSaveData::SceneEditorSaveData(SceneEditorTab* tab) : m_tab(tab)
{
}

void SceneEditorSaveData::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void SceneEditorSaveData::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void SceneEditorSaveData::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void SceneEditorSaveData::SerializeToJson(Serializer* serializer, json& j) const
{
	m_tab->WriteSaveDataToJson(serializer, j);
}

void SceneEditorSaveData::DeserializeFromJson(Serializer* serializer, const json& j)
{
	m_savedJson = std::move(j);
	//m_tab->ReadNodeDataFromJson(serializer, j);
}

Handle<ClassMetadata> SceneEditorSaveData::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void SceneEditorSaveData::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

