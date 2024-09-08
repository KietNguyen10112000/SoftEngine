#pragma once

#include "AnimLayer.h"
#include "Common/Math/Function1D.h"

#include <bitset>

NAMESPACE_BEGIN

class API AnimBlendLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimBlendLayer);

public:
	struct FLAG
	{
		enum ENUM
		{
			NO_AUTO_DISABLE = 1 << 0,
		};
	};

	AnimLayer* m_input[2] = {};

	float m_blendFactor = 0;
	float m_rangeMin = 0;
	float m_rangeMax = 1;
	float m_t = 0;

	// default: f(x) = x, range: 0 <= x <= 1
	SharedPtr<Function1D> m_controlFunction = std::make_shared<FunctionLinear1D>(1.0f, 0.0f);

	std::bitset<64> m_flags;

	mutable spinlock m_timeLock;

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
	void StartBlending(const SharedPtr<Function1D>& func1D, float timeMin, float timeMax);
	void Restart();

	void SetTime(float t);
	float GetTime() const;

	void SetFlag(FLAG::ENUM flag, bool enable);
	const std::bitset<64>& GetFlags() const;

	inline auto GetMainLayer() const
	{
		return m_input[(int)std::round(m_blendFactor)];
	}

	inline auto GetInput(ID index)
	{
		return m_input[index]->GetOutput();
	}

	inline const auto& GetControlFunction() const
	{
		return m_controlFunction;
	}

	inline auto GetTimeMin() const
	{
		return m_rangeMin;
	}

	inline auto GetTimeMax() const
	{
		return m_rangeMax;
	}

	inline auto GetBlendFactor() const
	{
		return std::clamp(GetControlFunction()->Test(GetTime()), 0.0f, 1.0f);
	}

};

NAMESPACE_END