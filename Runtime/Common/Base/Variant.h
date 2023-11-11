#pragma once

#include "Core/TypeDef.h"
#include "Math/Math.h"

NAMESPACE_BEGIN

class VARIANT_TYPE
{
public:
	enum TYPE
	{
		UNKNOWN,

		CHAR,
		INT8 = CHAR,

		UCHAR,
		UINT8 = UCHAR,

		INT16,
		UINT16,

		INT32,
		UINT32,

		INT64,
		UINT64,

		FLOAT,
		DOUBLE,

		VEC2,
		VEC3,
		VEC4,
		RGB,
		RGBA,
		QUATERNION,

		MAT3,
		MAT4,
		PROJECTION_MAT4,

		TRANSFORM3D,

		STRING,
		ARRAY_VARIANT
	};
};

class Variant
{
private:
	constexpr static size_t BUF_SIZE = sizeof(Mat4);

	VARIANT_TYPE::TYPE m_type;

	union
	{
		byte m_buffer[BUF_SIZE] = {};
		String m_str;
		mutable std::vector<Variant> m_array;
	};

public:
	Variant() : Variant(VARIANT_TYPE::UNKNOWN) {}

	Variant(VARIANT_TYPE::TYPE type) : m_type(type) 
	{
		SetType(type);
	}

	Variant(const Variant& another)
	{
		*this = another;
	}

	~Variant()
	{
		Reset();
	}

private:
	template <typename T>
	inline auto& Get() const
	{
		return *(T*)m_buffer;
	}

	inline void Reset()
	{
		::memset(m_buffer, 0, sizeof(m_buffer));

		if (m_type == VARIANT_TYPE::STRING)
		{
			m_str.~String();
		}

		if (m_type == VARIANT_TYPE::ARRAY_VARIANT)
		{
			m_array.~vector();
		}
	}

public:
	Variant& operator=(const Variant& another)
	{
		SetType(another.Type());

		if (m_type == VARIANT_TYPE::STRING)
		{
			m_str = another.AsString();
			goto Return;
		}

		if (m_type == VARIANT_TYPE::ARRAY_VARIANT)
		{
			m_array = another.AsArray();
			goto Return;
		}

		::memcpy(m_buffer, another.m_buffer, sizeof(m_buffer));
		
	Return:
		return *this;
	}

	template <typename T>
	inline static Variant Of()
	{
		if constexpr (std::is_same_v<T, char>)
		{
			return Variant(VARIANT_TYPE::CHAR);
		}

		if constexpr (std::is_same_v<T, uint8_t>)
		{
			return Variant(VARIANT_TYPE::UINT8);
		}

		if constexpr (std::is_same_v<T, int16_t>)
		{
			return Variant(VARIANT_TYPE::INT16);
		}

		if constexpr (std::is_same_v<T, uint16_t>)
		{
			return Variant(VARIANT_TYPE::UINT16);
		}

		if constexpr (std::is_same_v<T, int32_t>)
		{
			return Variant(VARIANT_TYPE::INT32);
		}

		if constexpr (std::is_same_v<T, uint32_t>)
		{
			return Variant(VARIANT_TYPE::UINT32);
		}

		if constexpr (std::is_same_v<T, int64_t>)
		{
			return Variant(VARIANT_TYPE::INT64);
		}

		if constexpr (std::is_same_v<T, uint64_t>)
		{
			return Variant(VARIANT_TYPE::UINT64);
		}

		if constexpr (std::is_same_v<T, float>)
		{
			return Variant(VARIANT_TYPE::FLOAT);
		}

		if constexpr (std::is_same_v<T, double>)
		{
			return Variant(VARIANT_TYPE::DOUBLE);
		}

		if constexpr (std::is_same_v<T, Vec2>)
		{
			return Variant(VARIANT_TYPE::VEC2);
		}

		if constexpr (std::is_same_v<T, Vec3>)
		{
			return Variant(VARIANT_TYPE::VEC3);
		}

		if constexpr (std::is_same_v<T, Vec4>)
		{
			return Variant(VARIANT_TYPE::VEC4);
		}

		if constexpr (std::is_same_v<T, Quaternion>)
		{
			return Variant(VARIANT_TYPE::QUATERNION);
		}

		if constexpr (std::is_same_v<T, Mat3>)
		{
			return Variant(VARIANT_TYPE::MAT3);
		}

		if constexpr (std::is_same_v<T, Mat4>)
		{
			return Variant(VARIANT_TYPE::MAT4);
		}

		if constexpr (std::is_same_v<T, Transform>)
		{
			return Variant(VARIANT_TYPE::TRANSFORM3D);
		}

		assert(0);
	}

	template <typename T>
	inline static Variant Of(const T& initValue)
	{
		Variant ret = Variant::Of<T>();
		ret.As<T>() = initValue;
		return ret;
	}

	inline const VARIANT_TYPE::TYPE& Type() const
	{
		return m_type;
	}

	template <typename T>
	inline auto& As() const
	{
		if constexpr (std::is_same_v<T, char>)
		{
			assert(Type() == VARIANT_TYPE::CHAR || Type() == VARIANT_TYPE::INT8);
			return Get<char>();
		}

		if constexpr (std::is_same_v<T, uint8_t>)
		{
			assert(Type() == VARIANT_TYPE::UINT8 || Type() == VARIANT_TYPE::UCHAR);
			return Get<uint8_t>();
		}

		if constexpr (std::is_same_v<T, int16_t>)
		{
			assert(Type() == VARIANT_TYPE::INT16);
			return Get<int16_t>();
		}

		if constexpr (std::is_same_v<T, uint16_t>)
		{
			assert(Type() == VARIANT_TYPE::UINT16);
			return Get<uint16_t>();
		}

		if constexpr (std::is_same_v<T, int32_t>)
		{
			assert(Type() == VARIANT_TYPE::INT32);
			return Get<int32_t>();
		}

		if constexpr (std::is_same_v<T, uint32_t>)
		{
			assert(Type() == VARIANT_TYPE::UINT32);
			return Get<uint32_t>();
		}

		if constexpr (std::is_same_v<T, int64_t>)
		{
			assert(Type() == VARIANT_TYPE::INT64);
			return Get<int64_t>();
		}

		if constexpr (std::is_same_v<T, uint64_t>)
		{
			assert(Type() == VARIANT_TYPE::UINT64);
			return Get<uint64_t>();
		}

		if constexpr (std::is_same_v<T, float>)
		{
			assert(Type() == VARIANT_TYPE::FLOAT);
			return Get<float>();
		}

		if constexpr (std::is_same_v<T, double>)
		{
			assert(Type() == VARIANT_TYPE::DOUBLE);
			return Get<double>();
		}

		if constexpr (std::is_same_v<T, Vec2>)
		{
			assert(Type() == VARIANT_TYPE::VEC2);
			return Get<Vec2>();
		}

		if constexpr (std::is_same_v<T, Vec3>)
		{
			assert(Type() == VARIANT_TYPE::VEC3 || Type() == VARIANT_TYPE::RGB);
			return Get<Vec3>();
		}

		if constexpr (std::is_same_v<T, Vec4>)
		{
			assert(Type() == VARIANT_TYPE::VEC4 || Type() == VARIANT_TYPE::RGBA);
			return Get<Vec4>();
		}

		if constexpr (std::is_same_v<T, Quaternion>)
		{
			assert(Type() == VARIANT_TYPE::QUATERNION);
			return Get<Quaternion>();
		}

		if constexpr (std::is_same_v<T, Mat3>)
		{
			assert(Type() == VARIANT_TYPE::MAT3);
			return Get<Mat3>();
		}

		if constexpr (std::is_same_v<T, Mat4>)
		{
			assert(Type() == VARIANT_TYPE::MAT4 || Type() == VARIANT_TYPE::PROJECTION_MAT4);
			return Get<Mat4>();
		}

		if constexpr (std::is_same_v<T, Transform>)
		{
			assert(Type() == VARIANT_TYPE::TRANSFORM3D);
			return Get<Transform>();
		}

		if constexpr (std::is_same_v<T, String>)
		{
			assert(Type() == VARIANT_TYPE::STRING);
			return AsString();
		}

		assert(0);
	}

	inline String AsString() const
	{
		assert(Type() == VARIANT_TYPE::STRING);
		return m_str;
	}

	inline std::vector<Variant>& AsArray() const
	{
		assert(Type() == VARIANT_TYPE::ARRAY_VARIANT);
		return m_array;
	}

	inline void SetType(VARIANT_TYPE::TYPE type)
	{
		Reset();
		m_type = type;
		switch (type)
		{
		case soft::VARIANT_TYPE::UNKNOWN:
			//assert(0);
			break;
		case soft::VARIANT_TYPE::CHAR:
		//case soft::VARIANT_TYPE::INT8:
		case soft::VARIANT_TYPE::UCHAR:
		//case soft::VARIANT_TYPE::UINT8:
		case soft::VARIANT_TYPE::INT16:
		case soft::VARIANT_TYPE::UINT16:
		case soft::VARIANT_TYPE::INT32:
		case soft::VARIANT_TYPE::UINT32:
		case soft::VARIANT_TYPE::INT64:
		case soft::VARIANT_TYPE::UINT64:
		case soft::VARIANT_TYPE::FLOAT:
		case soft::VARIANT_TYPE::DOUBLE:
		case soft::VARIANT_TYPE::VEC2:
		case soft::VARIANT_TYPE::VEC3:
		case soft::VARIANT_TYPE::VEC4:
		case soft::VARIANT_TYPE::RGB:
		case soft::VARIANT_TYPE::RGBA:
			Get<Vec4>() = { 0,0,0,0 };
			break;
		case soft::VARIANT_TYPE::QUATERNION:
			Get<Quaternion>() = {};
			break;
		case soft::VARIANT_TYPE::MAT3:
			Get<Mat3>() = {};
			break;
		case soft::VARIANT_TYPE::MAT4:
			Get<Mat4>() = {};
			break;
		case soft::VARIANT_TYPE::PROJECTION_MAT4:
			Get<Mat4>().SetPerspectiveFovLH(PI / 3.0f, 1, 0.5f, 1000.0f);
			break;
		case soft::VARIANT_TYPE::TRANSFORM3D:
			Get<Transform>() = {};
			break;
		case soft::VARIANT_TYPE::STRING:
			m_str = "";
			break;
		case soft::VARIANT_TYPE::ARRAY_VARIANT:
			new (&m_array) std::vector<Variant>();
			break;
		default:
			assert(0);
			break;
		}
	}

	
};

NAMESPACE_END