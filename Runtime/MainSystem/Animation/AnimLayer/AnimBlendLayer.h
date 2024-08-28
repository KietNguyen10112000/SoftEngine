#pragma once

#include "AnimLayer.h"
#include "Common/Math/Function1D.h"

NAMESPACE_BEGIN

class API AnimBlendLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimBlendLayer);

public:
	AnimLayer* m_input[2] = {};

	float m_blendFactor = 0;
	float m_rangeMin = 0;
	float m_rangeMax = 1;
	float m_t = 0;

	// default: f(x) = x, range: 0 <= x <= 1
	SharedPtr<Function1D> m_controlFunction = std::make_shared<FunctionLinear1D>(1.0f, 0.0f);

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
	void StartBlending(const SharedPtr<Function1D>& func1D, float rangeMin, float rangeMax);
	void Restart();

	void SetTime(float t);

	inline auto GetMainLayer() const
	{
		return m_input[(int)std::round(m_blendFactor)];
	}

};

NAMESPACE_END