#pragma once

#include "Core/Memory/SmartPointers.h"

#include "MainSystem/Physics/Components/PHYSICS_TYPE.h"

#include "Math/Math.h"

#include <vector>

#include "ComponentInspectorBase.h"

namespace soft
{
	class RigidBody;
	class ClassMetadata;
	class PhysicsShape;
	class GameObject;
}

using namespace soft;

class RigidBodyInspector : public ComponentInspectorBase
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

	float m_currentAlpha = 0.498f;

	int m_choosingCreateShapeIdx = 0;
	int m_countReloadShapeInspectorData = 0;

	Transform m_tempShapeLocalTransform = {};

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

	void InspectMaterials(PhysicsShape* shape);

	void DrawDebug(PhysicsShape* shape, const Vec4& color);

	void OnSelectShape(int idx);

	static void SetOpacityForObject(GameObject* o, float alpha);
public:
	void Inspect();

	virtual void OnBeginInspecting() override;
	virtual void OnEndInspecting() override;

};

