#pragma once

#include "PhysicsShape.h"

NAMESPACE_BEGIN

class PhysicsShapeUtils
{
	friend class PhysicsShapeBox;
	friend class PhysicsShapeCapsule;
	friend class PhysicsShapePlane;
	friend class PhysicsShapeSphere;

	template <typename PxGeo, typename S, typename M, typename... Args>
	inline static void InitializeShape(S* shape, const M& material, bool exclusive, Args&&... args)
	{
		auto physics = PhysX::Get()->GetPxPhysics();
		auto& m = *(material->m_pxMaterial);
		shape->m_pxShape = physics->createShape(PxGeo(std::forward<Args>(args)...), m, true);

		shape->m_pxShape->userData = shape;
		shape->m_meterial = material;

		shape->m_pxShape->acquireReference();
	}
};

NAMESPACE_END