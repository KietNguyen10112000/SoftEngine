#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class API AnimMixLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimMixLayer, SERIALIZABLE_MEM_RAW);

public:
	struct InputLayer
	{
	private:
		friend class AnimMixLayer;
		AnimLayer* outputLayer = nullptr;

	public:
		AnimLayer* layer = nullptr;
		std::vector<float> weight;
	};

	std::vector<InputLayer> m_inputs;

protected:
	// Inherited via AnimLayer
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void Run(float dt) override;

private:
	void AddInputImpl(AnimLayer*, const std::vector<float>& weight);
	void SetWeightImpl(ID index, const std::vector<float>& weight);

public:
	void AddInput(AnimLayer*, const std::vector<float>& weight);
	void SetWeight(ID index, const std::vector<float>& weight);

};

NAMESPACE_END