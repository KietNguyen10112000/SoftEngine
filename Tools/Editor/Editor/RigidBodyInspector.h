#pragma once

#include "Core/Memory/SmartPointers.h"

#include "MainSystem/Physics/Components/PHYSICS_TYPE.h"

#include <vector>

namespace soft
{
	class RigidBody;
	class ClassMetadata;
	class PhysicsShape;
}

using namespace soft;

class RigidBodyInspector
{
public:
	struct ShapeInspectorData
	{
		SharedPtr<PhysicsShape> shape;
	};

	ClassMetadata* m_metadata = nullptr;
	RigidBody* m_body = nullptr;

	PHYSICS_TYPE m_bodyType = PHYSICS_TYPE::PHYSICS_TYPE_RIGID_BODY_DYNAMIC;

	std::vector<ShapeInspectorData> m_shapeDatas;
	int m_choosingShapeIdx = 0;

public:
	RigidBodyInspector(RigidBody* body, ClassMetadata* metadata);

private:
	void InitializeNewShapeInspectorDatas(ShapeInspectorData* data, PhysicsShape* shape);
	void LoadShapeInspectorDatas();

	void InspectShapeBase(PhysicsShape* shape);
	void InspectShapeBox(PhysicsShape* shape);
	void InspectShapeCapsule(PhysicsShape* shape);
	void InspectShapePlane(PhysicsShape* shape);
	void InspectShapeSphere(PhysicsShape* shape);

public:
	void Inspect();

};

