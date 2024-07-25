#pragma once
#include "Common/Base/Serializable.h"

using namespace soft;

class SceneEditorTab;

class SceneEditorSaveData : public Serializable
{
public:
	SERIALIZABLE_CLASS(SceneEditorSaveData);

	SceneEditorTab* m_tab = nullptr;

	json m_savedJson;

	inline SceneEditorSaveData() {};
	SceneEditorSaveData(SceneEditorTab* tab);

public:
	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

};