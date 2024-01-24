#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/SmartPointers.h"
#include "Core/Structures/Structures.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class GameObject;

struct CollisionContactPoint
{
	Vec3 position;

	// normal from B to A
	Vec3 normal;

	Vec3 impluse;
};

struct CollisionContactPair
{
	GameObject* A;
	GameObject* B;
	std::vector<CollisionContactPoint> contacts;
};

struct Collision
{
	std::vector<SharedPtr<CollisionContactPair>> contactPairs;

	std::vector<ID> collisionBegin;
	std::vector<ID> collisionEnd;

	inline void Clear()
	{
		contactPairs.clear();
		collisionBegin.clear();
		collisionEnd.clear();
	}
};

NAMESPACE_END