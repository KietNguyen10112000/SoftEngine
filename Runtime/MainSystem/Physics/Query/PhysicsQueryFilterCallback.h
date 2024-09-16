#pragma once

#include "Core/Memory/Memory.h"

#include "Math/Math.h"

#include <bitset>

NAMESPACE_BEGIN

class GameObject;
class RigidBody;
class PhysicsShape;

struct PhysicsQueryHitLocation
{
	Vec3 position;
	Vec3 normal;

	GameObject* obj = nullptr;
	PhysicsShape* shape = nullptr;
};

struct PhysicsSweepHit : public PhysicsQueryHitLocation
{
	
};

struct PhysicsSweepResult
{
	std::vector<PhysicsSweepHit> touches;
	PhysicsSweepHit block;
	bool hasBlock = false;

	inline void Clear()
	{
		touches.clear();
		hasBlock = false;
	}
};

struct PhysicsOverlapHit : public PhysicsQueryHitLocation
{

};

struct PhysicsOverlapResult
{
	std::vector<PhysicsOverlapHit> touches;
	PhysicsOverlapHit block;
	bool hasBlock = false;

	inline void Clear()
	{
		touches.clear();
		hasBlock = false;
	}
};

struct PhysicsQueryHit
{

};

struct PhysicsHitFlag
{
	enum ENUM
	{
		DEFAULT
	};
};

using PhysicsHitFlags = std::bitset<sizeof(size_t)>;

#ifdef IGNORE
#undef IGNORE
#endif // IGNORE

struct PhysicsQueryHitType
{
	enum ENUM
	{
		IGNORE,

		// touch but not block
		TOUCH,

		// ray cast query should stop when BLOCK return
		BLOCK
	};
};

class PhysicsQueryFilterCallback
{
public:
	inline virtual ~PhysicsQueryFilterCallback() {};

	// this function should answer for the question that the "shape" of "obj" should be ignore or not
	virtual PhysicsQueryHitType::ENUM PrevFilter(GameObject* obj, PhysicsShape* shape, PhysicsHitFlags& flags) = 0;

	// post query filter
	virtual PhysicsQueryHitType::ENUM PostFilter(GameObject* obj, PhysicsShape* shape, const PhysicsQueryHit& hit) = 0;

};

NAMESPACE_END