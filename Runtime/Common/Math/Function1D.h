#pragma once

#include "Function.h"

NAMESPACE_BEGIN

// f(x) = ax + b;
class FunctionLinear1D : public Function1D
{
	SERIALIZABLE_CLASS(FunctionLinear1D, SERIALIZABLE_MEM_SHARED);
public:
	float m_a = 0;
	float m_b = 0;

	inline FunctionLinear1D()
	{

	}

	inline FunctionLinear1D(float a, float b)
	{
		Set(a, b);
	}

	inline FunctionLinear1D(const Vec2& p0, const Vec2& p1)
	{
		auto line = Line2D::FromPoints(p0, p1);
		assert(line.b != 0 && "Invalid function 1D!");

		m_a = line.a / line.b;
		m_b = line.c / line.b;
	}

	inline void Set(float a, float b)
	{
		m_a = a;
		m_b = b;
	}

	inline virtual float Test(float v) const override
	{
		return m_a * v + m_b;
	}

protected:
	void CloneFrom(Serializer* serializer, Serializable* another) override
	{
		auto* src = (FunctionLinear1D*)another;
		m_a = src->m_a;
		m_b = src->m_b;
	}

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
	{
	}

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
	{
	}

	void SerializeToJson(Serializer* serializer, json& j) const override
	{
		j["a"] = m_a;
		j["b"] = m_b;
	}

	void DeserializeFromJson(Serializer* serializer, const json& j) override
	{
		m_a = j["a"];
		m_b = j["b"];
	}

	Handle<ClassMetadata> GetMetadata(size_t sign) override
	{
		return nullptr;
	}

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
	{
	}

};

// f(x) = ax^2 + bx + c;
class FunctionQuadratic1D : public Function1D
{
	SERIALIZABLE_CLASS(FunctionQuadratic1D, SERIALIZABLE_MEM_SHARED);
public:
	float m_a = 0;
	float m_b = 0;
	float m_c = 0;

	inline FunctionQuadratic1D()
	{

	}

	inline FunctionQuadratic1D(float a, float b, float c)
	{
		Set(a, b, c);
	}

	inline void Set(float a, float b, float c)
	{
		m_a = a;
		m_b = b;
		m_c = c;
	}

	inline virtual float Test(float v) const override
	{
		return m_a * v * v + m_b * v + m_c;
	}

protected:
	void CloneFrom(Serializer* serializer, Serializable* another) override
	{
		auto* src = (FunctionQuadratic1D*)another;
		m_a = src->m_a;
		m_b = src->m_b;
		m_c = src->m_c;
	}

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
	{
	}

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
	{
	}

	void SerializeToJson(Serializer* serializer, json& j) const override
	{
		j["a"] = m_a;
		j["b"] = m_b;
		j["c"] = m_c;
	}

	void DeserializeFromJson(Serializer* serializer, const json& j) override
	{
		m_a = j["a"];
		m_b = j["b"];
		m_c = j["c"];
	}

	Handle<ClassMetadata> GetMetadata(size_t sign) override
	{
		return nullptr;
	}

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
	{
	}

};

NAMESPACE_END