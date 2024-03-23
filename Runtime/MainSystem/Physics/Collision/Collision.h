#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/SmartPointers.h"
#include "Core/Structures/Structures.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class GameObject;
class PhysicsMaterial;
class PhysicsShape;

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
	union 
	{
		struct 
		{
			PhysicsShape* AShape;
			PhysicsShape* BShape;
		};

		PhysicsShape* shapes[2];
	};

	std::vector<CollisionContactPoint> contactPoints;
};

struct CollisionEndContactPair
{
	union
	{
		struct
		{
			PhysicsShape* AShape;
			PhysicsShape* BShape;
		};

		PhysicsShape* shapes[2];
	};

	CollisionEndContactPair() : AShape(0), BShape(0) {};
	CollisionEndContactPair(PhysicsShape* AShape, PhysicsShape* BShape) : AShape(AShape), BShape(BShape) {};
};

struct CollisionEndContact
{
	union
	{
		struct
		{
			GameObject* A;
			GameObject* B;
		};

		GameObject* objects[2];
	};

	CollisionEndContact() : A(0), B(0) {};
	CollisionEndContact(GameObject* A, GameObject* B) : A(A), B(B) {};
};

struct CollisionContact
{
	GameObject* A;
	GameObject* B;
	std::vector<SharedPtr<CollisionContactPair>> contactPairs;
	std::vector<CollisionEndContactPair> endContactPairs;
	std::vector<uint32_t> endContactPairsIds;
	std::vector<uint32_t> beginContactPairsIds;

	// collisions still in touch, but the data is changed (eg: shape slides over another)
	std::vector<uint32_t> modifiedContactPairsIds;

	CollisionContact* oldCollisionContact = (CollisionContact*)INVALID_ID;

	//union
	//{
	//	//size_t newCollisionContactIdx = INVALID_ID;
	//	CollisionContact* oldCollisionContact;
	//};

	inline void Clear()
	{
		contactPairs.clear();

		endContactPairs.clear();
		endContactPairsIds.clear();
		beginContactPairsIds.clear();
	}
};

struct Collision
{
	std::vector<SharedPtr<CollisionContact>> contacts;

	std::vector<CollisionContact*> endContacts;

	size_t beginContactPairsCount = 0;
	size_t endContactPairsCount = 0;
	size_t contactPairsCount = 0;
	size_t contactPointsCount = 0;

	inline void Clear()
	{
		contacts.clear();
		endContacts.clear();
		beginContactPairsCount = 0;
		endContactPairsCount = 0;
		contactPairsCount = 0;
		contactPointsCount = 0;
	}
};

NAMESPACE_END