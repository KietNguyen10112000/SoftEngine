#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class AnimBlendLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimBlendLayer);

private:
	AnimLayer* m_input[2] = {};
	uint32_t m_currentLayerId = 0;

	float m_blendTime = 0;
	float m_blendTotalTime = 0;

protected:
	// Inherited via AnimLayer
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void Run(float dt) override;

public:
	virtual AnimLayer* GetOutput() override;

	void SetInput(AnimLayer* l1, AnimLayer* l2);
	void FadeTo(ID animationId, float startTime, float endTime, float fadeTime);

};

NAMESPACE_END