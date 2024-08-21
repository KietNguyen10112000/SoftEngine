#pragma once

#include "Core/Memory/SmartPointers.h"

#include "MainSystem/Physics/Components/PHYSICS_TYPE.h"

#include "Math/Math.h"

#include <vector>
#include <map>

#include "ComponentInspectorBase.h"

namespace soft
{
	class RigidBody;
	class ClassMetadata;
	class PhysicsShape;
	class GameObject;
	class Joint;
}

using namespace soft;

class RigidBodyInspector : public ComponentInspectorBase
{
public:
	struct ShapeInspectorData
	{
		SharedPtr<PhysicsShape> shape;

		bool deleted = false;
		bool locked = true;
	};

	struct JointInspectorData
	{
		Joint* joint = nullptr;
	};

	struct DrawShapeData
	{
		SharedPtr<PhysicsShape> shape = nullptr;
		Mat4 globalTransformMat;
		Vec4 color;
		bool showBasis;
	};

	ClassMetadata* m_metadata = nullptr;
	RigidBody* m_body = nullptr;

	PHYSICS_TYPE m_bodyType = PHYSICS_TYPE::PHYSICS_TYPE_RIGID_BODY_DYNAMIC;

	std::vector<ShapeInspectorData> m_shapeDatas;
	int m_choosingShapeIdx = -1;

	float m_currentAlpha = 0.498f;

	int m_choosingCreateShapeIdx = 0;
	int m_countReloadShapeInspectorData = 0;

	Transform m_tempShapeLocalTransform = {};

	bool m_hideAllOtherShapes = false;

	std::vector<JointInspectorData> m_jointDatas;
	int m_choosingJointIdx = -1;
	int m_choosingCreateJointIdx = 0;
	int m_countReloadJointInspectorData = 0;
	RigidBody* m_jointCreateAnother = nullptr;
	char m_jointCreateSearchName[256] = {};
	Transform m_tempJointTransform = {};
	int m_tempJointTransformCount = 0;

	uint32_t m_tempCollisionMask = 0;
	bool m_tempIsEnableFamilyNoCollide = false;

	std::map<PhysicsShape*, DrawShapeData> m_currentDrawData;
	Handle<GameObject> m_debugJointAnotherObject = nullptr;

	bool m_isHoveringFamilyScale = false;
	bool m_prevDrawDebugAllBodiesFromRoot = true;
	bool m_isDrawDebugAllBodiesFromRoot = true;
	bool m_isDrawBasis = false;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_debugJointAnotherObject);
	}

public:
	RigidBodyInspector(RigidBody* body, ClassMetadata* metadata);

private:
	void InitializeNewShapeInspectorDatas(ShapeInspectorData* data, PhysicsShape* shape);
	void LoadShapeInspectorDatas();
	void InitializeNewJointInspectorDatas(JointInspectorData* data, Joint* joint);
	void LoadJointInspectorDatas();
	void FindJointCreateAnother();

	void InspectShapeBase(PhysicsShape* shape);
	void InspectShapeBox(PhysicsShape* shape);
	void InspectShapeCapsule(PhysicsShape* shape);
	void InspectShapePlane(PhysicsShape* shape);
	void InspectShapeSphere(PhysicsShape* shape);

	void InspectMaterials(PhysicsShape* shape);

	void RenderInspectShape();

	bool InspectJointLimitBase(void* limit, Joint* joint);

	void InspectJointBase(Joint* joint);
	void InspectJointFixed(Joint* joint);
	void InspectJointSpherical(Joint* joint);
	void InspectJointRevolute(Joint* joint);
	void InspectJointD6(Joint* joint);
	void RenderInspectJoint();

	void DrawDebugImpl(const Mat4& globalTransformMat, PhysicsShape* shape, const Vec4& color, bool showBasis);
	void DrawDebug(RigidBody* body, PhysicsShape* shape, const Vec4& color, bool showBasis, bool isDebugJointAnotherObject);
	void FlushDrawDebug();

	void OnSelectShape(int idx);
	void OnSelectJoint(int idx);

	void ScaleBodyFromRootObject(float scaleFactor);
	void DrawDebugShapeFromRoot();

public:
	void Inspect();

	virtual void OnBeginInspecting() override;
	virtual void OnEndInspecting() override;

};

