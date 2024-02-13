#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/SmartPointers.h"
#include "Core/Structures/Structures.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class GameObject;
class PhysicsMaterial;

struct CollisionContactPoint
{
	Vec3 position;

	// normal from B to A
	Vec3 normal;

	Vec3 impulse;

	float dynamicFriction;
	float staticFriction;

	//SharedPtr<PhysicsMaterial> ASurfaceMaterial;
	//SharedPtr<PhysicsMaterial> BSurfaceMaterial;
};

struct CollisionContactPair
{
	GameObject* A;
	GameObject* B;
	std::vector<CollisionContactPoint> contacts;
};

struct CollisionEndContactPair
{
	GameObject* A;
	GameObject* B;
};

struct Collision
{
	std::vector<SharedPtr<CollisionContactPair>> contactPairs;
	std::vector<CollisionEndContactPair> endContactPairs;

	std::vector<ID> collisionEnd;
	size_t collisionBeginCount = 0;

	inline void Clear()
	{
		contactPairs.clear();
		endContactPairs.clear();
	}
};

NAMESPACE_END