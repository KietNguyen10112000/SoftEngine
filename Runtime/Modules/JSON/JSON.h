#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include "Math/Math.h"

#include "nlohmann/json.hpp"

using json = ::nlohmann::json;

//NAMESPACE_BEGIN

//namespace JSON
//{

namespace math
{

inline void to_json(json& j, const Vec3& vec)
{
	j = json::array();
	j.push_back(vec.x);
	j.push_back(vec.y);
	j.push_back(vec.z);
}

inline void from_json(const json& j, Vec3& vec)
{
	vec.x = j[0];
	vec.y = j[1];
	vec.z = j[2];
}

inline void to_json(json& j, const Vec4& vec)
{
	j = json::array();
	j.push_back(vec.x);
	j.push_back(vec.y);
	j.push_back(vec.z);
	j.push_back(vec.w);
}

inline void from_json(const json& j, Vec4& vec)
{
	vec.x = j[0];
	vec.y = j[1];
	vec.z = j[2];
	vec.w = j[3];
}

inline void to_json(json& ret, const Quaternion& quaternion)
{
	ret = json::array();
	ret.push_back(quaternion.x);
	ret.push_back(quaternion.y);
	ret.push_back(quaternion.z);
	ret.push_back(quaternion.w);
}

inline void from_json(const json& j, Quaternion& quaternion)
{
	quaternion.x = j[0];
	quaternion.y = j[1];
	quaternion.z = j[2];
	quaternion.w = j[3];
}

inline void to_json(json& ret, const Mat4& mat)
{
	ret = json::array();
	for (int y = 0; y < 4; y++)
	{
		json row = json::array();
		for (int x = 0; x < 4; x++)
		{
			row.push_back(mat[y][x]);
		}
	}
}

inline void from_json(const json& ret, Mat4& mat)
{
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			mat[y][x] = ret[y][x];
		}
	}
}

inline void to_json(json& ret, const Transform& transform)
{
	ret["scale"]	= transform.GetScale();
	ret["rotation"] = transform.GetRotation();
	ret["position"] = transform.GetPosition();
}

inline void from_json(const json& ret, Transform& transform)
{
	transform.Scale()	 = ret["scale"];
	transform.Rotation() = ret["rotation"];
	transform.Position() = ret["position"];
}

//}

}

//NAMESPACE_END