#include "GameObjectEditorSaveData.h"

#include "GameObjectEditorTab.h"

GameObjectEditorSaveData::GameObjectEditorSaveData(GameObjectEditorTab* tab) : Base(tab)
{
}

void GameObjectEditorSaveData::CloneFrom(Serializer* serializer, Serializable* another)
{
	Base::CloneFrom(serializer, another);
}

void GameObjectEditorSaveData::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
	Base::SerializeToBinary(serializer, stream);
}

void GameObjectEditorSaveData::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
	Base::DeserializeFromBinary(serializer, stream);
}

void GameObjectEditorSaveData::SerializeToJson(Serializer* serializer, json& j) const
{
	Base::SerializeToJson(serializer, j);
}

void GameObjectEditorSaveData::DeserializeFromJson(Serializer* serializer, const json& j)
{
	Base::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> GameObjectEditorSaveData::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void GameObjectEditorSaveData::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}
