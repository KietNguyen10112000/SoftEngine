#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class API AnimJointLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimJointLayer);

public:
	struct InputLayer
	{
	private:
		friend class AnimJointLayer;
		AnimLayer* outputLayer = nullptr;

	public:
		AnimLayer* layer = nullptr;
		std::vector<bool> mask;
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
	void AddInputImpl(AnimLayer*, const std::vector<bool>& mask);
	void SetMaskImpl(ID index, const std::vector<bool>& mask);

public:
	void AddInput(AnimLayer*, const std::vector<bool>& mask);
	void SetMask(ID index, const std::vector<bool>& mask);

};

NAMESPACE_END