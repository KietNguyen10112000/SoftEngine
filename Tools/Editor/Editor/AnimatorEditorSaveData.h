#pragma once
#include "Common/Base/Serializable.h"

using namespace soft;

class AnimatorEditorTab;

class AnimatorEditorSaveData : public Serializable
{
public:
	SERIALIZABLE_CLASS(AnimatorEditorSaveData);

	struct NodeData
	{
		ID nodeId = INVALID_ID;
		Vec2 position;
		UUID layerUUID;
	};

	UUID m_objectUUID;
	UUID m_sceneUUID;
	UUID m_cameraUUID;
	String m_name;

	AnimatorEditorTab* m_tab = nullptr;

	json m_savedJson;

	inline AnimatorEditorSaveData() {};
	AnimatorEditorSaveData(AnimatorEditorTab* tab);

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