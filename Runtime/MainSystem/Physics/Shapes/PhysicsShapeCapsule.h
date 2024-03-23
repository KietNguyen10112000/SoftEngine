#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeCapsule : public PhysicsShape
{
private:
	friend class CharacterControllerCapsule;
	PhysicsShapeCapsule(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);

	inline static SharedPtr<PhysicsShapeCapsule> 
		MakeDummy(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false)
	{
		struct make_shared_enabler : public PhysicsShapeCapsule 
		{
			make_shared_enabler(void* pxShape, float height, float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive)
				: PhysicsShapeCapsule(pxShape, height, radius, material, isExclusive) {};
		};
		return std::make_shared<make_shared_enabler>(pxShape, height, radius, material, isExclusive);
	}

public:
	PhysicsShapeCapsule(float height, float radius, const SharedPtr<PhysicsMaterial>& material, bool isExclusive = false);


	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_CAPSULE;
	};
};

NAMESPACE_END