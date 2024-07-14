#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include "UUID/UUID.h"

#include "Math/Math.h"

#ifdef snprintf
#undef snprintf
#endif // snprintf

#include "Libraries/nlohmann/single_include/nlohmann/json.hpp"

#include <cstdio>

using json = ::nlohmann::json;

//NAMESPACE_BEGIN

//namespace JSON
//{

namespace math
{

inline void to_json(json& j, const Vec2& vec)
{
	j = json::array();
	j.push_back(vec.x);
	j.push_back(vec.y);
}

inline void from_json(const json& j, Vec2& vec)
{
	vec.x = j[0];
	vec.y = j[1];
}

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
		ret.push_back(row);
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

inline void to_json(json& ret, const Line& line)
{
	ret["point"]		= line.m_point;
	ret["direction"]	= line.m_direction;
}

inline void from_json(const json& ret, Line& line)
{
	line.m_point		= ret["point"];
	line.m_direction	= ret["direction"];
}

inline void to_json(json& ret, const Plane& plane)
{
	ret["a"] = plane.a;
	ret["b"] = plane.b;
	ret["c"] = plane.c;
	ret["d"] = plane.d;
}

inline void from_json(const json& ret, Plane& plane)
{
	plane.a = ret["a"];
	plane.b = ret["b"];
	plane.c = ret["c"];
	plane.d = ret["d"];
}

inline void to_json(json& ret, const Sphere& sphere)
{
	ret["center"] = sphere.m_center;
	ret["radius"] = sphere.m_radius;
}

inline void from_json(const json& ret, Sphere& sphere)
{
	sphere.m_center = ret["center"];
	sphere.m_radius = ret["radius"];
}

inline void to_json(json& ret, const Box& box)
{
	ret["position"] = box.m_position;
	ret["d1"]		= box.m_d1;
	ret["d2"]		= box.m_d2;
	ret["d3"]		= box.m_d3;
}

inline void from_json(const json& ret, Box& box)
{
	box.m_position	= ret["position"];
	box.m_d1		= ret["d1"];
	box.m_d2		= ret["d2"];
	box.m_d3		= ret["d3"];
}

inline void to_json(json& ret, const AABox& aaBox)
{
	ret["center"]			= aaBox.m_center;
	ret["halfDimensions"]	= aaBox.m_halfDimensions;
}

inline void from_json(const json& ret, AABox& aaBox)
{
	aaBox.m_center			= ret["center"];
	aaBox.m_halfDimensions	= ret["halfDimensions"];
}

inline void to_json(json& ret, const Capsule& capsule)
{
	ret["center"]	= capsule.m_center;
	ret["radius"]	= capsule.m_radius;
	ret["height"]	= capsule.m_height;
	ret["up"]		= capsule.m_up;
}

inline void from_json(const json& ret, Capsule& capsule)
{
	capsule.m_center	= ret["center"];
	capsule.m_radius	= ret["radius"];
	capsule.m_height	= ret["height"];
	capsule.m_up		= ret["up"];
}

inline void to_json(json& ret, const Frustum& frustum)
{
	ret["p0"] = frustum.m_planes[0];
	ret["p1"] = frustum.m_planes[1];
	ret["p2"] = frustum.m_planes[2];
	ret["p3"] = frustum.m_planes[3];
	ret["p4"] = frustum.m_planes[4];
	ret["p5"] = frustum.m_planes[5];
}

inline void from_json(const json& ret, Frustum& frustum)
{
	frustum.m_planes[0] = ret["p0"];
	frustum.m_planes[1] = ret["p1"];
	frustum.m_planes[2] = ret["p2"];
	frustum.m_planes[3] = ret["p3"];
	frustum.m_planes[4] = ret["p4"];
	frustum.m_planes[5] = ret["p5"];
}

//}

}

//NAMESPACE_END

NAMESPACE_BEGIN

inline void to_json(json& ret, const String& str)
{
	if (str.empty())
	{
		ret = "";
		return;
	}
	ret = str.c_str();
}

inline void from_json(const json& ret, String& str)
{
	auto s = ret.get<json::string_t>();
	if (!s.empty())
	{
		str = s.c_str();
	}
}

inline void to_json(json& ret, const UUID& uuid)
{
	ret = uuid.ToHexString().c_str();
}

inline void from_json(const json& ret, UUID& uuid)
{
	uuid = UUID::FromHexString(ret.get<json::string_t>().c_str());
}

NAMESPACE_END