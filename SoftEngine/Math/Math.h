#pragma once
#include <cmath>
#include <iostream>
#include <string>
#include <DirectXMath.h>

using namespace DirectX;

namespace Math
{

#define PI 3.14159265359f

	inline float ConvertToRadians(float fDegrees)
	{
		return fDegrees * (XM_PI / 180.0f);
	}

	inline float ConvertToDegrees(float fRadians)
	{
		return fRadians * (180.0f / XM_PI);
	}

	//typedef XMFLOAT4X4 Mat4x4;
	//typedef XMFLOAT3X3 Mat3x3;
	//typedef XMFLOAT4 Vec4;
	//typedef XMFLOAT3 Vec3;
	typedef XMFLOAT2 Vec2;

	template<typename T>
	inline Vec2 operator*(const Vec2& vec, T i)
	{
		Vec2 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		return re;
	}

	template<typename T>
	inline Vec2 operator*(T i, const Vec2& vec)
	{
		Vec2 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		return re;
	}

	inline Vec2 operator*(const Vec2& v1, const Vec2& v2)
	{
		Vec2 re;
		re.x = v1.x * v2.x;
		re.y = v1.y * v2.y;
		return re;
	}

	inline Vec2 operator+(const Vec2& vec1, const Vec2& vec2)
	{
		Vec2 re;
		re.x = vec1.x + vec2.x;
		re.y = vec1.y + vec2.y;
		return re;
	}

	inline Vec2 operator-(const Vec2& vec1, const Vec2& vec2)
	{
		Vec2 re;
		re.x = vec1.x - vec2.x;
		re.y = vec1.y - vec2.y;
		return re;
	}

	class Vec3 : public XMFLOAT3
	{
	public:
		Vec3() : XMFLOAT3() {};
		Vec3(float x, float y, float z) : XMFLOAT3(x, y, z) {};

		/*inline void operator=(const Vec3& vec)
		{
			XMStoreFloat3(this, XMLoadFloat3(&vec));
		}*/

		inline bool operator==(const Vec3& vec)
		{
			return vec.x == x && vec.y == y && vec.z == z;
		}

		inline void operator+=(const Vec3& vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		inline void operator-=(const Vec3& vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		inline Vec3& Normalize()
		{
			XMStoreFloat3(this, XMVector3Normalize(XMLoadFloat3(this)));
			return *this;
		}

		inline Vec3 Normal() const
		{
			Vec3 ret;
			XMStoreFloat3(&ret, XMVector3Normalize(XMLoadFloat3(this)));
			return ret;
		}

		inline float Length() const
		{
			return sqrt(x * x + y * y + z * z);
		}

		inline std::string ToString() const
		{
			std::string ret = "Vec3("
				+ std::to_string(x)
				+ ", " + std::to_string(y)
				+ ", " + std::to_string(z) + ")";

			return ret;
		};
	};

	template<typename T>
	inline Vec3 operator*(const Vec3& vec, T i)
	{
		Vec3 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		re.z = vec.z * i;
		return re;
	}

	template<typename T>
	inline Vec3 operator*(T i, const Vec3& vec)
	{
		Vec3 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		re.z = vec.z * i;
		return re;
	}

	template<typename T>
	inline Vec3 operator/(const Vec3& vec, T i)
	{
		Vec3 re;
		re.x = vec.x / i;
		re.y = vec.y / i;
		re.z = vec.z / i;
		return re;
	}

	template<typename T>
	inline Vec3 operator/(T i, const Vec3& vec)
	{
		Vec3 re;
		re.x = i / vec.x;
		re.y = i / vec.y;
		re.z = i / vec.z;
		return re;
	}

	inline Vec3 operator*(const Vec3& v1, const Vec3& v2)
	{
		Vec3 re;
		re.x = v1.x * v2.x;
		re.y = v1.y * v2.y;
		re.z = v1.z * v2.z;
		return re;
	}

	inline Vec3 operator/(const Vec3& v1, const Vec3& v2)
	{
		Vec3 re;
		re.x = v1.x / v2.x;
		re.y = v1.y / v2.y;
		re.z = v1.z / v2.z;
		return re;
	}

	inline Vec3 operator+(const Vec3& vec1, const Vec3& vec2)
	{
		Vec3 re;
		re.x = vec1.x + vec2.x;
		re.y = vec1.y + vec2.y;
		re.z = vec1.z + vec2.z;
		return re;
	}

	inline Vec3 operator-(const Vec3& vec1, const Vec3& vec2)
	{
		Vec3 re;
		re.x = vec1.x - vec2.x;
		re.y = vec1.y - vec2.y;
		re.z = vec1.z - vec2.z;
		return re;
	}

	class Vec4 : public XMFLOAT4
	{
	public:
		Vec4() : XMFLOAT4() {};
		Vec4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {};

		Vec4(const Vec3& vec3, float w) : XMFLOAT4(vec3.x, vec3.y, vec3.z, w) {};

		/*inline void operator=(const Vec4& vec)
		{
			XMStoreFloat4(this, XMLoadFloat4(&vec));
		}*/

		inline bool operator==(const Vec4& vec)
		{
			return vec.x == x && vec.y == y && vec.z == z && vec.w == w;
		}

		inline void operator+=(const Vec4& vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
			w += vec.w;
		}

		inline void operator-=(const Vec4& vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
			w -= vec.w;
		}

		inline Vec4& Normalize()
		{
			XMStoreFloat4(this, XMVector4Normalize(XMLoadFloat4(this)));
			return *this;
		}

		inline float Length()
		{
			return sqrt(x * x + y * y + z * z + w * w);
		}

		inline std::string ToString() const
		{
			std::string ret = "Vec4("
				+ std::to_string(x)
				+ ", " + std::to_string(y)
				+ ", " + std::to_string(z) 
				+ ", " + std::to_string(w)
				+ ")";

			return ret;
		};

	};

	template<typename T>
	inline Vec4 operator*(const Vec4& vec, T i)
	{
		Vec4 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		re.z = vec.z * i;
		re.w = vec.w * i;
		return re;
	}

	template<typename T>
	inline Vec4 operator*(T i, const Vec4& vec)
	{
		Vec4 re;
		re.x = vec.x * i;
		re.y = vec.y * i;
		re.z = vec.z * i;
		re.w = vec.w * i;
		return re;
	}

	template<typename T>
	inline Vec4 operator/(const Vec4& vec, T i)
	{
		Vec4 re;
		re.x = vec.x / i;
		re.y = vec.y / i;
		re.z = vec.z / i;
		re.w = vec.w / i;
		return re;
	}

	template<typename T>
	inline Vec4 operator/(T i, const Vec4& vec)
	{
		Vec4 re;
		re.x = i / vec.x;
		re.y = i / vec.y;
		re.z = i / vec.z;
		re.w = i / vec.w;
		return re;
	}

	inline Vec4 operator*(const Vec4& v1, const Vec4& v2)
	{
		Vec4 re;
		re.x = v1.x * v2.x;
		re.y = v1.y * v2.y;
		re.z = v1.z * v2.z;
		re.w = v1.w * v2.w;
		return re;
	}

	inline Vec4 operator/(const Vec4& v1, const Vec4& v2)
	{
		Vec4 re;
		re.x = v1.x / v2.x;
		re.y = v1.y / v2.y;
		re.z = v1.z / v2.z;
		re.w = v1.w / v2.w;
		return re;
	}

	inline Vec4 operator+(const Vec4& vec1, const Vec4& vec2)
	{
		Vec4 re;
		re.x = vec1.x + vec2.x;
		re.y = vec1.y + vec2.y;
		re.z = vec1.z + vec2.z;
		re.w = vec1.w + vec2.w;
		return re;
	}

	inline Vec4 operator-(const Vec4& vec1, const Vec4& vec2)
	{
		Vec4 re;
		re.x = vec1.x - vec2.x;
		re.y = vec1.y - vec2.y;
		re.z = vec1.z - vec2.z;
		re.w = vec1.w - vec2.w;
		return re;
	}

	class Mat4x4 : public XMFLOAT4X4
	{
	public:
		//default is identity matrix
		inline Mat4x4()
		{
			XMStoreFloat4x4(this, XMMatrixIdentity());
		}

		inline Mat4x4(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4)
		{
			Vec4* p = (Vec4*)&m[0];
			*p = v1;
			p = (Vec4*)&m[1];
			*p = v2;
			p = (Vec4*)&m[2];
			*p = v3;
			p = (Vec4*)&m[3];
			*p = v4;
		}

		/*inline void operator=(const Mat4x4& mat)
		{
			XMStoreFloat4x4(this, XMLoadFloat4x4(&mat));
		}*/

		inline void operator*=(const Mat4x4& mat)
		{
			XMStoreFloat4x4(
				this,
				XMMatrixMultiply(XMLoadFloat4x4(this), XMLoadFloat4x4(&mat))
				//XMMatrixMultiply(XMLoadFloat4x4(&mat), XMLoadFloat4x4(this))
			);
		}

		inline void operator+=(const Mat4x4& mat)
		{
			float* mem = &m[0][0];
			const float* mem1 = &mat.m[0][0];

			for (size_t i = 0; i < 16; i++)
			{
				mem[i] += mem1[i];
			}
		}

		inline Mat4x4& Inverse()
		{
			XMStoreFloat4x4(
				this,
				XMMatrixInverse(NULL, XMLoadFloat4x4(this))
			);
			return *this;
		}

		inline Vec3 GetPosition()
		{
			return Vec3(_41, _42, _43);
		}

		//only some case
		inline Vec3 GetScale()
		{
			Vec3 re;
			re.x = Vec3(_11, _21, _31).Length();
			re.y = Vec3(_12, _22, _32).Length();
			re.z = Vec3(_13, _23, _33).Length();
			return re;
		}

		inline Vec4 GetRotationQuaternion()
		{
			Vec4 re;
			XMStoreFloat4(&re, XMQuaternionRotationMatrix(XMLoadFloat4x4(this)));
			return re;
		}

		inline Vec3 GetRotationAngle()
		{
			float rx = std::atan2(_23, _33);
			float ry = std::atan2(-_13, std::sqrt(_23 * _23 + _33 * _33)); //std::asin(-_13);
			float rz = std::atan2(_12, _11);
			return Vec3(rx, ry, rz);
		}

		inline Vec3 GetForwardDir()
		{
			//return Vec3(_13, _23, _33);
			return Vec3(_31, _32, _33);
		}

		inline Vec3 GetRightwardDir()
		{
			//return Vec3(_11, _21, _31);
			return Vec3(_11, _12, _13);
		}

		inline Vec3 GetBackwardDir()
		{
			return Vec3(-_31, -_32, -_33);
		}

		inline Vec3 GetLeftwardDir()
		{
			return Vec3(-_11, -_12, -_13);
		}

		inline Vec3 GetUpwardDir()
		{
			return Vec3(_21, _22, _23);
		}

		inline Mat4x4& SetIdentity()
		{
			XMStoreFloat4x4(this, XMMatrixIdentity());
			return *this;
		}

		inline Mat4x4& SetRotationX(float angle)
		{
			XMStoreFloat4x4(this, XMMatrixRotationX(angle));
			return *this;
		}

		inline Mat4x4& SetRotationY(float angle)
		{
			XMStoreFloat4x4(this, XMMatrixRotationY(angle));
			return *this;
		}

		inline Mat4x4& SetRotationZ(float angle)
		{
			XMStoreFloat4x4(this, XMMatrixRotationZ(angle));
			return *this;
		}

		inline Mat4x4& SetRotation(Vec4& quaternion)
		{
			XMStoreFloat4x4(this, XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion)));
			return *this;
		}

		inline Mat4x4& SetRotation(const Vec4& quaternion)
		{
			XMStoreFloat4x4(this, XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion)));
			return *this;
		}

		inline Mat4x4& SetTranslation(float x, float y, float z)
		{
			XMStoreFloat4x4(this, XMMatrixTranslation(x, y, z));
			return *this;
		}

		inline Mat4x4& SetTranslation(Vec3& vec)
		{
			XMStoreFloat4x4(this, XMMatrixTranslation(vec.x, vec.y, vec.z));
			return *this;
		}

		inline Mat4x4& SetTranslation(const Vec3& vec)
		{
			XMStoreFloat4x4(this, XMMatrixTranslation(vec.x, vec.y, vec.z));
			return *this;
		}

		inline Mat4x4& SetPosition(float x, float y, float z)
		{
			_41 = x;
			_42 = y;
			_43 = z;
			return *this;
		}

		inline Mat4x4& SetPosition(const Vec3& vec)
		{
			_41 = vec.x;
			_42 = vec.y;
			_43 = vec.z;
			return *this;
		}

		inline Mat4x4& SetScale(float scaleX, float scaleY, float scaleZ)
		{
			XMStoreFloat4x4(this, XMMatrixScaling(scaleX, scaleY, scaleZ));
			return *this;
		}

		inline Mat4x4& SetScale(Vec3& vec)
		{
			XMStoreFloat4x4(this, XMMatrixScaling(vec.x, vec.y, vec.z));
			return *this;
		}

		inline Mat4x4& SetScale(const Vec3& vec)
		{
			XMStoreFloat4x4(this, XMMatrixScaling(vec.x, vec.y, vec.z));
			return *this;
		}

		inline Mat4x4& Transpose()
		{
			XMStoreFloat4x4(this, XMMatrixTranspose(XMLoadFloat4x4(this)));
			return *this;
		}

		inline Mat4x4& SetLookAtRH(const Vec3& position, const Vec3& focusPos, const Vec3& up)
		{
			XMStoreFloat4x4(this, XMMatrixLookAtRH(
				XMLoadFloat3(&position),
				XMLoadFloat3(&focusPos),
				XMLoadFloat3(&up)
			));
			return *this;
		}

		inline Mat4x4& SetLookAtLH(const Vec3& position, const Vec3& focusPos, const Vec3& up)
		{
			XMStoreFloat4x4(this, XMMatrixLookAtLH(
				XMLoadFloat3(&position),
				XMLoadFloat3(&focusPos),
				XMLoadFloat3(&up)
			));
			return *this;
		}

		//fov must be in radians
		inline Mat4x4& SetPerspectiveFovLH(float fov, float dNear, float dFar, float aspectRatio)
		{
			XMStoreFloat4x4(this, DirectX::XMMatrixPerspectiveFovLH(
				fov,
				aspectRatio,
				dNear,
				dFar
			));
			return *this;
		}

		inline Mat4x4& SetOrthographicLH(float viewWith, float viewHeight, float nearZ, float farZ)
		{
			XMStoreFloat4x4(this, DirectX::XMMatrixOrthographicLH(
				viewWith,
				viewHeight,
				nearZ,
				farZ
			));
			return *this;
		}

		inline Mat4x4& RemoveTranslation()
		{
			m[3][0] = 0;
			m[3][1] = 0;
			m[3][2] = 0;
			m[3][3] = 1;
			m[0][3] = 0;
			m[1][3] = 0;
			m[2][3] = 0;
			return *this;
		}

		inline void Decompose(Vec3* outScale, Vec4* outRotationQua, Vec3* outTranslation)
		{
			XMVECTOR p, s, r;
			XMMatrixDecompose(&s, &r, &p, XMLoadFloat4x4(this));
			XMStoreFloat3(outScale, s);
			XMStoreFloat4(outRotationQua, r);
			XMStoreFloat3(outTranslation, p);
		}

		inline std::string ToString() const
		{
			std::string ret = "Mat4x4(\n";

			for (size_t i = 0; i < 4; i++)
			{
				ret += "\t[";
				for (size_t j = 0; j < 4; j++)
				{
					ret += std::to_string(m[i][j]) + ", ";
				}
				ret.resize(ret.size() - 2);
				ret += "]\n";
			}

			ret += "\trow major\n)";

			return ret;
		};

	};

	template<typename T>
	inline Mat4x4 operator*(const Mat4x4& mat, T left)
	{
		Mat4x4 re;
		float* mem = &re.m[0][0];
		const float* mem1 = &mat.m[0][0];

		for (size_t i = 0; i < 16; i++)
		{
			mem[i] = mem1[i] * left;
		}

		return re;
	}

	template<typename T>
	inline Mat4x4 operator/(const Mat4x4& mat, T left)
	{
		Mat4x4 re;
		float* mem = &re.m[0][0];
		const float* mem1 = &mat.m[0][0];

		for (size_t i = 0; i < 16; i++)
		{
			mem[i] = mem1[i] / left;
		}

		return re;
	}

	inline Mat4x4 operator*(Mat4x4& mat1, Mat4x4& mat2)
	{
		Mat4x4 re;
		XMStoreFloat4x4(
			&re,
			XMMatrixMultiply(XMLoadFloat4x4(&mat1), XMLoadFloat4x4(&mat2))
		);
		return re;
	}

	inline Mat4x4 operator*(const Mat4x4& mat1, const Mat4x4& mat2)
	{
		Mat4x4 re;
		XMStoreFloat4x4(
			&re,
			XMMatrixMultiply(XMLoadFloat4x4(&mat1), XMLoadFloat4x4(&mat2))
		);
		return re;
	}

	inline Vec4 operator*(const Vec4& vec, const Mat4x4& mat)
	{
		Vec4 re;
		XMStoreFloat4(
			&re,
			XMVector4Transform(XMLoadFloat4(&vec), XMLoadFloat4x4(&mat))
		);
		return re;
	}

	inline Mat4x4 operator+(const Mat4x4& mat1, const Mat4x4& mat2)
	{
		Mat4x4 re;

		float* mem = &re.m[0][0];
		const float* mem1 = &mat1.m[0][0];
		const float* mem2 = &mat2.m[0][0];

		for (size_t i = 0; i < 16; i++)
		{
			mem[i] = mem1[i] + mem2[i];
		}
		
		return re;
	}

	inline Mat4x4 operator-(const Mat4x4& mat1, const Mat4x4& mat2)
	{
		Mat4x4 re;

		float* mem = &re.m[0][0];
		const float* mem1 = &mat1.m[0][0];
		const float* mem2 = &mat2.m[0][0];

		for (size_t i = 0; i < 16; i++)
		{
			mem[i] = mem1[i] - mem2[i];
		}

		return re;
	}

	inline Mat4x4 GetTranslationMatrix(float x, float y, float z)
	{
		Mat4x4 re;
		return re.SetTranslation(x, y, z);
	}

	inline Mat4x4 GetTranslationMatrix(Vec3& vec)
	{
		Mat4x4 re;
		return re.SetTranslation(vec);
	}

	inline Mat4x4 GetTranslationMatrix(const Vec3& vec)
	{
		Mat4x4 re;
		return re.SetTranslation(vec);
	}

	inline Mat4x4 GetRotationMatrix(const Vec4& quaternion)
	{
		Mat4x4 re;
		return re.SetRotation(quaternion);
	}

	inline Mat4x4 GetRotationMatrix(float roll, float pitch, float yaw)
	{
		Mat4x4 re;
		XMStoreFloat4x4(&re, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
		return re;
	}

	inline Mat4x4 GetRotationMatrix(const Vec3& axis, float angle)
	{
		Mat4x4 re;
		XMStoreFloat4x4(&re, XMMatrixRotationAxis(XMLoadFloat3(&axis), angle));
		return re;
	}

	inline Mat4x4 GetRotationXMatrix(float angle)
	{
		Mat4x4 re;
		return re.SetRotationX(angle);
	}

	inline Mat4x4 GetRotationYMatrix(float angle)
	{
		Mat4x4 re;
		return re.SetRotationY(angle);
	}

	inline Mat4x4 GetRotationZMatrix(float angle)
	{
		Mat4x4 re;
		return re.SetRotationZ(angle);
	}

	inline Mat4x4 GetScaleMatrix(float sX, float sY, float sZ)
	{
		Mat4x4 re;
		return re.SetScale(sX, sY, sZ);
	}

	inline Mat4x4 GetScaleMatrix(const Vec3& vec)
	{
		Mat4x4 re;
		return re.SetScale(vec);
	}

	inline Mat4x4 GetInverse(const Mat4x4& mat)
	{
		Mat4x4 re = mat;
		return re.Inverse();
	}

	inline Vec4 SLerp(Vec4& vec1, Vec4& vec2, float delta)
	{
		Vec4 re;
		XMStoreFloat4(&re, XMQuaternionSlerp(XMLoadFloat4(&vec1), XMLoadFloat4(&vec2), delta));
		return re;
	}

	inline Vec4 Lerp(Vec4& vec1, Vec4& vec2, float delta)
	{
		Vec4 re;
		XMStoreFloat4(&re, XMVectorLerp(XMLoadFloat4(&vec1), XMLoadFloat4(&vec2), delta));
		return re;
	}

	inline Vec3 Lerp(Vec3& vec1, Vec3& vec2, float delta)
	{
		Vec3 re;
		XMStoreFloat3(&re, XMVectorLerp(XMLoadFloat3(&vec1), XMLoadFloat3(&vec2), delta));
		return re;
	}

	inline Mat4x4 QuaternionToMatrix(Vec4& quaternion)
	{
		Mat4x4 re;
		XMStoreFloat4x4(&re, XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion)));
		return re;
	}

	inline Vec3 QuaternionToEulerAngles(const Vec4& q)
	{
		Vec3 angles;

		// roll (x-axis rotation)
		double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
		double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
		angles.x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		double sinp = 2 * (q.w * q.y - q.z * q.x);
		if (std::abs(sinp) >= 1)
			angles.y = std::copysign(PI / 2, sinp); // use 90 degrees if out of range
		else
			angles.y = std::asin(sinp);

		// yaw (z-axis rotation)
		double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		angles.z = std::atan2(siny_cosp, cosy_cosp);

		return angles;
	}

	class Mat3x3 : public XMFLOAT3X3
	{
	public:
		//default is identity matrix
		inline Mat3x3()
		{
			XMStoreFloat3x3(this, XMMatrixIdentity());
		}

		inline Mat3x3(const Vec3& v1, const Vec3& v2, const Vec3& v3)
		{
			Vec3* p = (Vec3*)&m[0];
			*p = v1;
			p = (Vec3*)&m[1];
			*p = v2;
			p = (Vec3*)&m[2];
			*p = v3;
		}

		/*inline void operator=(const Mat3x3& mat)
		{
			XMStoreFloat3x3(this, XMLoadFloat3x3(&mat));
		}*/

		inline void operator*=(const Mat3x3& mat)
		{
			XMStoreFloat3x3(
				this,
				XMMatrixMultiply(XMLoadFloat3x3(this), XMLoadFloat3x3(&mat))
			);
		}

		inline Vec2 GetForwardDir()
		{
			return Vec2(_13, _23);
		}

		inline Vec2 GetRightwardDir()
		{
			return Vec2(_11, _21);
		}

		inline Vec2 GetUpwardDir()
		{
			return Vec2(_12, _22);
		}

		inline void SetRotationX(float angle)
		{
			XMStoreFloat3x3(this, XMMatrixRotationX(angle));
		}

		inline void SetRotationY(float angle)
		{
			XMStoreFloat3x3(this, XMMatrixRotationY(angle));
		}

		inline void SetRotationZ(float angle)
		{
			XMStoreFloat3x3(this, XMMatrixRotationZ(angle));
		}

		inline void SetTranslation(float x, float y)
		{
			XMStoreFloat3x3(this, XMMatrixTranslation(x, y, 0));
		}

		inline void SetTranslation(Vec2& vec)
		{
			XMStoreFloat3x3(this, XMMatrixTranslation(vec.x, vec.y, 0));
		}

		inline void SetTranslation(const Vec2& vec)
		{
			XMStoreFloat3x3(this, XMMatrixTranslation(vec.x, vec.y, 0));
		}

	};

	inline Mat3x3 operator*(const Mat3x3& mat1, const Mat3x3& mat2)
	{
		Mat3x3 re;
		XMStoreFloat3x3(
			&re,
			XMMatrixMultiply(XMLoadFloat3x3(&mat1), XMLoadFloat3x3(&mat2))
		);
		return re;
	}

	inline Vec3 operator*(const Vec3& vec, const Mat3x3& mat)
	{
		Vec3 re;
		XMStoreFloat3(
			&re,
			XMVector3Transform(XMLoadFloat3(&vec), XMLoadFloat3x3(&mat))
		);
		return re;
	}

	inline float GetAngleBetween(const Vec3& v1, const Vec3& v2)
	{
		float re;
		XMStoreFloat(&re, XMVector3AngleBetweenVectors(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
		return re;
	}

	inline float DotProduct(const Vec3& v1, const Vec3& v2)
	{
		float re;
		XMStoreFloat(&re, XMVector3Dot(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
		//return v1.x * v2.x + 
		return re;
	}

	inline float DotProduct(const Vec4& v1, const Vec4& v2)
	{
		float re;
		XMStoreFloat(&re, XMVector4Dot(XMLoadFloat4(&v1), XMLoadFloat4(&v2)));
		return re;
	}

	inline Vec3 CrossProduct(const Vec3& v1, const Vec3& v2)
	{
		Vec3 re;
		XMStoreFloat3(&re, XMVector3Cross(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
		return re;
	}

	//return Quaternion that rotate angle degrees around axis
	inline Vec4 GetRotationAxis(const Vec3& axis, float angle)
	{
		Vec4 re;
		XMStoreFloat4(&re, XMQuaternionRotationAxis(XMLoadFloat3(&axis), angle));
		return re;
	}

	inline Mat4x4 GetRotationAxisMatrix(const Vec3& axis, float angle)
	{
		Mat4x4 re;
		XMStoreFloat4x4(&re, XMMatrixRotationAxis(XMLoadFloat3(&axis), angle));
		return re;
	}

	//return Quaternion that is shortest rotation {from->to}
	//source from https://github.com/toji/gl-matrix/blob/f0583ef53e94bc7e78b78c8a24f09ed5e2f7a20c/src/gl-matrix/quat.js#L54 line 54
	inline Vec4 GetQuaternionFrom(const Vec3& from, const Vec3& to)
	{
		Vec3 xUnitVec3 = { 1, 0, 0 };
		Vec3 yUnitVec3 = { 0, 1, 0 };
		float dot = DotProduct(from, to);

		if (dot < -0.999999) {
			Vec3 temp = CrossProduct(xUnitVec3, from);
			if (temp.Length() < 0.000001)
				temp = CrossProduct(yUnitVec3, from);

			temp.Length();
			//quat.setAxisAngle(out, tmpvec3, Math.PI);
			return GetRotationAxis(temp, PI);
		}
		else if (dot > 0.999999) {
			return { 0,0,0,1 };
		}
		else {
			Vec4 re;
			Vec3 temp = CrossProduct(from, to);
			re.x = temp.x;
			re.y = temp.y;
			re.z = temp.z;
			re.w = 1 + dot;
			return re.Normalize();
		}
	}

	inline Mat3x3 ConvertMatrix(const Mat4x4& mat)
	{
		Vec3* v1 = (Vec3*)&mat.m[0];
		Vec3* v2 = (Vec3*)&mat.m[1];
		Vec3* v3 = (Vec3*)&mat.m[2];
		return { *v1,*v2,*v3 };
	}

	inline Mat4x4 ConvertMatrix(const Mat3x3& mat)
	{
		Vec3* v1 = (Vec3*)&mat.m[0];
		Vec3* v2 = (Vec3*)&mat.m[1];
		Vec3* v3 = (Vec3*)&mat.m[2];

		Mat4x4 re = {};

		Vec3* _v1 = (Vec3*)&re.m[0];
		Vec3* _v2 = (Vec3*)&re.m[1];
		Vec3* _v3 = (Vec3*)&re.m[2];

		*_v1 = *v1;
		*_v2 = *v2;
		*_v3 = *v3;

		return re;
	}

	//get xyz as vec3
	inline Vec3 ConvertVector(const Vec4& vec)
	{
		Vec3 re;
		re = *((Vec3*)&vec);
		return re;
	}

	inline Vec4 ConvertVector(const Vec3& vec, float w = 0)
	{
		Vec4 re = {};
		*((Vec3*)&re) = vec;
		re.w = w;
		return re;
	}

	inline std::ostream& operator<<(std::ostream& os, const Vec3& vec)
	{
		os << "Vec3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const Vec4& vec)
	{
		os << "Vec4(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const Mat4x4& vec)
	{
		os << vec.ToString();
		return os;
	}

}

using namespace Math;