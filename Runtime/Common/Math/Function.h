#pragma once

#include "Math/Fundamental.h"

#include "Common/Base/Serializable.h"

NAMESPACE_BEGIN

//namespace math
//{

class Function1D : public Serializable
{
public:
	virtual float Test(float v) const = 0;

};

class Function2D : public Serializable
{
public:
	virtual float Test(const Vec2& v) const = 0;

};

class Function3D : public Serializable
{
public:
	virtual float Test(const Vec3& v) const = 0;

};

//}

NAMESPACE_END