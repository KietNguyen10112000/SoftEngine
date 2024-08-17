#include "RigidBodyInspector.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/RigidBodyStatic.h"

#include "MainSystem/Physics/Materials/PhysicsMaterial.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeSphere.h"
#include "MainSystem/Physics/Joints/FixedJoint.h"
#include "MainSystem/Physics/Joints/SphericalJoint.h"
#include "MainSystem/Physics/Joints/RevoluteJoint.h"
#include "MainSystem/Physics/Joints/D6Joint.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "Scene/GameObject.h"

#include "imgui/imgui.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "DataInspector.h"
#include "EditorContext.h"
#include "SceneEditorTab.h"
#include "ImGuiExtern.h"

RigidBodyInspector::RigidBodyInspector(RigidBody* body, ClassMetadata* metadata) : ComponentInspectorBase(body), m_body(body), m_metadata(metadata)
{
	m_bodyType = m_body->GetPhysicsType();

	LoadShapeInspectorDatas();
	LoadJointInspectorDatas();
}

void RigidBodyInspector::InitializeNewShapeInspectorDatas(ShapeInspectorData* data, PhysicsShape* shape)
{
	data->shape = shape->shared_from_this();
}

void RigidBodyInspector::LoadShapeInspectorDatas()
{
	std::map<PhysicsShape*, ShapeInspectorData*> oldShapes;
	for (auto& data : m_shapeDatas)
	{
		oldShapes.insert({ data.shape.get(),&data });
	}

	std::vector<ShapeInspectorData> newShapeDatas;

	auto count = m_body->GetShapesCount();
	for (size_t i = 0; i < count; i++)
	{
		auto shape = m_body->GetShape(i);
		
		auto it = oldShapes.find(shape);
		if (it != oldShapes.end())
		{
			newShapeDatas.push_back(*(it->second));
		}
		else
		{
			InitializeNewShapeInspectorDatas(&newShapeDatas.emplace_back(), shape);
		}
	}

	m_shapeDatas.swap(newShapeDatas);

	if (m_shapeDatas.size() != 0 && m_choosingShapeIdx >= 0)
	{
		OnSelectShape(m_choosingShapeIdx);
	}
}

void RigidBodyInspector::InitializeNewJointInspectorDatas(JointInspectorData* data, Joint* joint)
{
	data->joint = joint;
}

void RigidBodyInspector::LoadJointInspectorDatas()
{
	std::map<Joint*, JointInspectorData*> oldJoints;
	for (auto& data : m_jointDatas)
	{
		oldJoints.insert({ data.joint,&data });
	}

	std::vector<JointInspectorData> newJointDatas;

	auto count = m_body->GetJointsCount();
	for (size_t i = 0; i < count; i++)
	{
		auto& joint = m_body->GetJoint(i);

		auto it = oldJoints.find(joint.Get());
		if (it != oldJoints.end())
		{
			newJointDatas.push_back(*(it->second));
		}
		else
		{
			InitializeNewJointInspectorDatas(&newJointDatas.emplace_back(), joint.Get());
		}
	}

	m_jointDatas.swap(newJointDatas);

	if (m_jointDatas.size() != 0 && m_choosingJointIdx >= 0)
	{
		OnSelectJoint(m_choosingJointIdx);
	}
}

void RigidBodyInspector::FindJointCreateAnother()
{
	m_jointCreateAnother = nullptr;
	auto name = m_jointCreateSearchName;
	m_body->GetGameObject()->GetRoot()->PostTraversal(
		[&](GameObject* o)
		{
			if (o->Name() == name && o->HasComponent<RigidBody>() && m_jointCreateAnother == nullptr)
			{
				m_jointCreateAnother = o->GetComponentRaw<RigidBody>();
			}
		}
	);
}

void RigidBodyInspector::InspectShapeBase(PhysicsShape* shape)
{
	ImGui::TextUnformatted("Shape Local Transform");

	Vec3 rotationAxis = Vec3::ZERO;
	auto modified = m_tempShapeLocalTransform;
	auto accessor = Accessor::For("Transform", modified, m_body);
	DataInspector::InspectTransformEx(m_metadata, accessor, accessor.Get(), String::Format("ShapeLocalTransform {}", shape).c_str(), true, &rotationAxis);
	
	auto dbGr = Graphics::Get()->GetDebugGraphics();
	if (dbGr && rotationAxis != Vec3::ZERO)
	{
		Transform globalTransform = {};
		m_body->GetGameObject()->GetCommittedGlobalTransform().Decompose(globalTransform.Scale(), globalTransform.Rotation(), globalTransform.Position());

		// physics component doesn't use scale component
		globalTransform.Scale() = { 1,1,1 };

		auto localPose = shape->GetLocalTransform();
		auto globalMat = localPose.ToTransformMatrix() * globalTransform.ToTransformMatrix();

		Vec4 color = { 0,0,0,1 };
		auto localMat = localPose.ToTransformMatrix();
		if (rotationAxis.Equals(localMat.Right().Normal(), 0.001f))
		{
			rotationAxis = globalMat.Right().Normal();
			color.x = 1.0f;
		}

		if (rotationAxis.Equals(localMat.Up().Normal(), 0.001f))
		{
			rotationAxis = globalMat.Up().Normal();
			color.y = 1.0f;
		}

		if (rotationAxis.Equals(localMat.Forward().Normal(), 0.001f))
		{
			rotationAxis = globalMat.Forward().Normal();
			color.z = 1.0f;
		}

		dbGr->DrawLineSegment(
			globalMat.Position() - rotationAxis * 10.0f,
			globalMat.Position() + rotationAxis * 10.0f,
			color,
			0.02f
		);
	}
	
	if (!m_tempShapeLocalTransform.Equals(modified))
	{
		shape->SetLocalTransform(modified);
		m_tempShapeLocalTransform = modified;
	}
}

void RigidBodyInspector::InspectShapeBox(PhysicsShape* shape)
{
	auto box = (PhysicsShapeBox*)shape;
	auto dimensions = box->GetDimensions();
	const char* names[] = { "X", "Y", "Z" };
	ImGui::TextUnformatted("Box Dimensions");
	if (ImGui::DragFloatNEx(names, &dimensions[0], 3, 0.01f, -INFINITY, INFINITY))
	{
		box->SetDimensions(dimensions);
	}
}

void RigidBodyInspector::InspectShapeCapsule(PhysicsShape* shape)
{
	auto capsule = (PhysicsShapeCapsule*)shape;
	auto r = capsule->GetRadius();
	auto h = capsule->GetHeight();
	const char* names[] = { "Radius", "Height" };
	Vec2 temp = { r,h };
	if (ImGui::DragFloatNEx(names, &temp[0], 2, 0.01f, -INFINITY, INFINITY))
	{
		capsule->SetRadius(temp.x);
		capsule->SetHeight(temp.y);
	}
}

void RigidBodyInspector::InspectShapePlane(PhysicsShape* shape)
{
}

void RigidBodyInspector::InspectShapeSphere(PhysicsShape* shape)
{
	auto sphere = (PhysicsShapeSphere*)shape;
	auto r = sphere->GetRadius();
	const char* names[] = { "Radius" };
	if (ImGui::DragFloatNEx(names, &r, 1, 0.01f, -INFINITY, INFINITY))
	{
		sphere->SetRadius(r);
	}
}

void RigidBodyInspector::InspectMaterials(PhysicsShape* shape)
{
	auto& m = shape->GetFirstMaterial();

	auto staticFriction = m->GetStaticFriction();
	auto dynamicFriction = m->GetDynamicFriction();
	auto restitution = m->GetRestitution();

	ImGui::TextUnformatted("Material");

	if (ImGui::DragFloat("Static Friction", &staticFriction, 0.001f, 0.001f, 1.0f))
	{
		m->SetStaticFriction(staticFriction);
	}

	if (ImGui::DragFloat("Dynamic Friction", &dynamicFriction, 0.001f, 0.001f, 1.0f))
	{
		m->SetDynamicFriction(dynamicFriction);
	}

	if (ImGui::DragFloat("Restitution", &restitution, 0.001f, 0.001f, 1.0f))
	{
		m->SetRestitution(restitution);
	}
}

void RigidBodyInspector::RenderInspectShape()
{
	static const char* s_shapeList[] = {
		"Sphere",
		"Capsule",
		"Box",
		"Plane",
	};

	ImGui::BeginChild(ID(this), { ImGui::GetWindowWidth() * 0.88f, 0 }, true);

	if (ImGui::Button("+ Add Shape"))
	{
		m_choosingCreateShapeIdx = 0;

		EditorContext::DialogDesc desc;
		desc.title = "Add Physics Shape";
		EditorContext::Get()->OpenOkCancelDialog(desc,
			[](void* p)
			{
				auto self = (RigidBodyInspector*)p;

				if (ImGui::BeginCombo("Choose Shape Type", s_shapeList[self->m_choosingCreateShapeIdx]))
				{
					for (size_t n = 0; n < IM_ARRAYSIZE(s_shapeList); n++)
					{
						ImGui::PushID(n);
						if (ImGui::Selectable(s_shapeList[n]))
						{
							self->m_choosingCreateShapeIdx = int(n);
						}
						ImGui::PopID();
					}

					ImGui::EndCombo();
				}

			}, this,
			[](EditorContext::DIALOG_RESULT result, void* p) -> bool
				{
					auto self = (RigidBodyInspector*)p;
					if (result == EditorContext::OK)
					{
						auto* sampleMaterial = self->m_body->GetShapesCount() == 0 ? nullptr : self->m_body->GetShape(0)->GetFirstMaterial().get();

						SharedPtr<PhysicsShape> shape = nullptr;
						SharedPtr<PhysicsMaterial> material = std::make_shared<PhysicsMaterial>(
							sampleMaterial == nullptr ? 0.5f : sampleMaterial->GetStaticFriction(),
							sampleMaterial == nullptr ? 0.5f : sampleMaterial->GetDynamicFriction(),
							sampleMaterial == nullptr ? 0.5f : sampleMaterial->GetRestitution()
						);
						switch (self->m_choosingCreateShapeIdx)
						{
						case 0:
							shape = std::make_shared<PhysicsShapeSphere>(1.0f, material);
							break;
						case 1:
							shape = std::make_shared<PhysicsShapeCapsule>(2.0f, 1.0f, material);
							break;
						case 2:
							shape = std::make_shared<PhysicsShapeBox>(Vec3(1.0f, 1.0f, 1.0f), material);
							break;
						case 3:
							shape = std::make_shared<PhysicsShapePlane>(material);
							break;
						default:
							break;
						}

						if (shape)
						{
							self->m_body->AddShape(shape);
							self->m_countReloadShapeInspectorData = 2;
						}
					}
					return true;
				}, this
				);
	}

	ImGui::SameLine();
	float scaleOffset = 0;
	ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 279);
	if (ImGui::DragFloat("Unique Scale", &scaleOffset, 0.001f, -INFINITY, INFINITY, ""))
	{
		m_body->ScaleBy(1 + scaleOffset);
	}

	if (m_body->GetShapesCount() == 0 || m_shapeDatas.size() != m_body->GetShapesCount() || m_choosingShapeIdx < 0)
	{
		if (m_body->GetShapesCount() != 0 && m_shapeDatas.size() != 0)
		{
			OnSelectShape(0);
		}

		ImGui::EndChild();
		return;
	}

	String previewText = String::Format("[{}] {}", m_choosingShapeIdx, m_choosingShapeIdx >= 0 ? s_shapeList[m_shapeDatas[m_choosingShapeIdx].shape->GetType()] : "");
	if (ImGui::BeginCombo("Choose Shape", m_choosingShapeIdx >= 0 ? previewText.c_str() : nullptr))
	{
		for (size_t n = 0; n < m_shapeDatas.size(); n++)
		{
			String text = String::Format("[{}] {}", n, s_shapeList[m_shapeDatas[n].shape->GetType()]);
			ImGui::PushID(n);
			if (ImGui::Selectable(text.c_str()))
			{
				m_choosingShapeIdx = int(n);
				OnSelectShape(m_choosingShapeIdx);
			}
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetItemUsingMouseWheel();

		if (ImGui::GetIO().MouseWheel)
		{
			int incre = ImGui::GetIO().MouseWheel < 0 ? 1 : int(m_shapeDatas.size() - 1);
			m_choosingShapeIdx = (m_choosingShapeIdx + incre) % m_shapeDatas.size();
			OnSelectShape(m_choosingShapeIdx);
		}
	}

	ImGui::Separator();

	PhysicsShape* deleteShape = nullptr;
	auto shapesCount = m_body->GetShapesCount();
	if (m_choosingShapeIdx >= 0)
	{
		auto shape = m_body->GetShape(m_choosingShapeIdx);
		auto& shapeData = m_shapeDatas[m_choosingShapeIdx];
		auto type = shape->GetType();

		auto typeName = shape->GetClassName();

		InspectMaterials(shape);

		ImGui::SetNextItemOpen(true);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.f));
		auto open = ImGui::TreeNodeEx(typeName, ImGuiTreeNodeFlags_FramePadding);
		ImGui::PopStyleVar();

		if (shapesCount >= 2 && shapeData.deleted == false)
		{
			auto pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 36, pos.y - 35));
			if (ImGui::Button(ICON_FA_TRASH " ## delete shape btn", ImVec2(30, 30)))
			{
				deleteShape = shape;
				shapeData.deleted = true;
			}

			ImGui::SetCursorPos(pos);
		}


		if (open)
		{
			ImGui::Checkbox("Lock", &shapeData.locked);

			ImGui::SameLine(0, 25);
			if (ImGui::Button(ICON_FA_CLONE " Clone"))
			{
				Serializer serializer = {};
				auto clonedShape = serializer.Clone(shape)->shared_from_this();
				m_body->AddShape(clonedShape);
				m_countReloadShapeInspectorData = 2;
			}

			ImGui::SameLine(0, 25);
			ImGui::Checkbox("Hide Others", &m_hideAllOtherShapes);

			ImGui::BeginDisabled(shapeData.locked);

			InspectShapeBase(shape);

			ImGui::Dummy({ 10,10 });
			ImGui::Separator();
			//ImGui::Dummy({ 7,7 });

			DrawDebug(m_body, shape, Vec4(0, 1, 0, 1), true, false);

			switch (type)
			{
			case PHYSICS_SHAPE_TYPE_SPHERE:
				InspectShapeSphere(shape);
				break;
			case PHYSICS_SHAPE_TYPE_CAPSULE:
				InspectShapeCapsule(shape);
				break;
			case PHYSICS_SHAPE_TYPE_BOX:
				InspectShapeBox(shape);
				break;
			case PHYSICS_SHAPE_TYPE_PLANE:
				InspectShapePlane(shape);
				break;
			case PHYSICS_SHAPE_TYPE_CONVEX_MESH:
				break;
			case PHYSICS_SHAPE_TYPE_TRIANGLE_MESH:
				break;
			default:
				break;
			}

			ImGui::EndDisabled();
			ImGui::TreePop();
		}
	}

	if (!m_hideAllOtherShapes)
	{
		for (size_t i = 0; i < shapesCount; i++)
		{
			if (i != m_choosingShapeIdx)
			{
				DrawDebug(m_body, m_body->GetShape(i), Vec4(0.8f, 0, 0, 1), false, false);
			}
		}
	}

	ImGui::EndChild();

	if (deleteShape)
	{
		m_body->RemoveShape(deleteShape);
		m_countReloadShapeInspectorData = 2;

		m_choosingShapeIdx = clamp(m_choosingShapeIdx, 0, int(shapesCount) - 2);
		OnSelectShape(m_choosingShapeIdx);
	}
}

bool RigidBodyInspector::InspectJointLimitBase(void* pLimit, Joint* joint)
{
	bool modified = false;
	auto& limit = *(Joint::BaseLimit*)pLimit;
	if (ImGui::DragFloat("Restitution", &limit.restitution, 0.001f, 0.0, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
	{
		modified = true;
	}

	if (ImGui::DragFloat("BounceThreshold", &limit.bounceThreshold, 0.001f, 0.0, INFINITY, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
	{
		modified = true;
	}

	if (ImGui::DragFloat("Stiffness", &limit.stiffness, 0.001f, 0.0, INFINITY))
	{
		modified = true;
	}

	if (ImGui::DragFloat("Damping", &limit.damping, 0.001f, 0.0, INFINITY))
	{
		modified = true;
	}
	return modified;
}

void RigidBodyInspector::InspectJointBase(Joint* joint)
{
	if (m_tempJointTransformCount != 0)
	{
		m_tempJointTransformCount--;
	}

	auto jointGlobalTransform = joint->GetGlobalTransform();
	if (!jointGlobalTransform.Equals(m_tempJointTransform, 0.01f) && m_tempJointTransformCount == 0)
	{
		m_tempJointTransform = jointGlobalTransform;
	}

	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		auto temp = jointGlobalTransform;
		temp.Scale() = { 0.01f,0.01f,0.01f };
		debugGraphics->DrawCube(temp.ToTransformMatrix(), {1,1,0,1});

		auto mat = jointGlobalTransform.ToTransformMatrix();
		debugGraphics->DrawLineSegment(mat.Position(), mat.Position() + mat.Forward().Normal() * 0.2f, { 0,0,1,1 }, 0.01f);
		debugGraphics->DrawLineSegment(mat.Position(), mat.Position() + mat.Right().Normal() * 0.2f, { 1,0,0,1 }, 0.01f);
		// the oriention of joint follows X Axis
		debugGraphics->DrawLineSegment(mat.Position() + mat.Right().Normal() * 0.2f, mat.Position() + mat.Right().Normal() * 0.26f, { 1,0,0,1 }, 0.03f);
		debugGraphics->DrawLineSegment(mat.Position(), mat.Position() + mat.Up().Normal() * 0.2f, { 0,1,0,1 }, 0.01f);
	}

	ImGui::TextUnformatted("Joint Transform");
	auto modified = m_tempJointTransform;
	auto accessor = Accessor::For("Transform", modified, m_body);
	bool v = DataInspector::InspectTransformEx(m_metadata, accessor, accessor.Get(), String::Format("JointGlobalTransform {}", joint).c_str(), true);
	if (v)
	{
		auto a0GlobalTransform = joint->GetBody0()->GetGameObject()->GetCommittedGlobalTransform();
		auto a1GlobalTransform = joint->GetBody1()->GetGameObject()->GetCommittedGlobalTransform();

		auto temp0 = Transform::FromTransformMatrix(a0GlobalTransform);
		temp0.Scale() = { 1,1,1 };
		auto temp1 = Transform::FromTransformMatrix(a1GlobalTransform);
		temp1.Scale() = { 1,1,1 };

		modified.Scale() = { 1,1,1 };
		auto jointGlobalTransform = modified.ToTransformMatrix();

		auto localframe0 = Transform::FromTransformMatrix(jointGlobalTransform * temp0.ToTransformMatrix().GetInverse());
		auto localframe1 = Transform::FromTransformMatrix(jointGlobalTransform * temp1.ToTransformMatrix().GetInverse());

		joint->SetLocalFrame(joint->GetBody0(), localframe0);
		joint->SetLocalFrame(joint->GetBody1(), localframe1);

		m_tempJointTransform = modified;
		m_tempJointTransformCount = 5;
	}

	{
		ImGui::Dummy({ 5, 15 });
		auto f = joint->GetBreakForce();
		auto tq = joint->GetBreakTorque();
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 250);
		if (ImGui::DragFloat("Break Force Length", &f, 0.001f, 0.001f, INFINITY, f == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			joint->SetBreakForce(f, tq);
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 50);
		if (ImGui::Button(ICON_FA_ROTATE " ## reset break force"))
		{
			joint->SetBreakForce(FLT_MAX, tq);
		}

		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 250);
		if (ImGui::DragFloat("Break Torque Length", &tq, 0.001f, 0.001f, INFINITY, tq == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			joint->SetBreakForce(f, tq);
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 50);
		if (ImGui::Button(ICON_FA_ROTATE " ## reset break torque"))
		{
			joint->SetBreakForce(f, FLT_MAX);
		}
	}
	
}

void RigidBodyInspector::InspectJointFixed(Joint* joint)
{
}

void RigidBodyInspector::InspectJointSpherical(Joint* _joint)
{
	auto joint = (SphericalJoint*)_joint;

	{
		auto limit = joint->GetLimit();
		auto enableLimit = joint->IsEnableLimit();
		if (ImGui::Checkbox("Enable Limit", &enableLimit))
		{
			joint->SetEnableLimit(enableLimit);
		}

		ImGui::BeginDisabled(!enableLimit);

		bool modified = InspectJointLimitBase(&limit, joint);

		ImGui::Dummy({ 5,10 });
		if (ImGui::DragFloat("Cone Angle Y", &limit.yLimitAngle, 0.001f, 0.0, PI, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}
		if (ImGui::DragFloat("Cone Angle Z", &limit.zLimitAngle, 0.001f, 0.0, PI, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		if (modified)
		{
			joint->SetLimit(limit);
		}
		ImGui::EndDisabled();
	}
}

void RigidBodyInspector::InspectJointRevolute(Joint* _joint)
{
	auto jointGlobalTransform = _joint->GetGlobalTransform();

	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		auto mat = jointGlobalTransform.ToTransformMatrix();
		debugGraphics->DrawLineSegment(mat.Position() - mat.Right().Normal(), mat.Position() + mat.Right().Normal(), { 1,0,0,1 }, 0.005f);
	}

	auto joint = (RevoluteJoint*)_joint;

	{
		auto limit = joint->GetLimit();
		auto enableLimit = joint->IsEnableLimit();
		if (ImGui::Checkbox("Enable Limit", &enableLimit))
		{
			joint->SetEnableLimit(enableLimit);
		}

		ImGui::BeginDisabled(!enableLimit);

		bool modified = InspectJointLimitBase(&limit, joint);

		ImGui::Dummy({ 5,10 });
		if (ImGui::DragFloat("Upper", &limit.upperLimit, 0.001f, limit.lowerLimit, PI / 2.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}
		if (ImGui::DragFloat("Lower", &limit.lowerLimit, 0.001f, -PI / 2.0f, limit.upperLimit, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		if (modified)
		{
			joint->SetLimit(limit);
		}
		ImGui::EndDisabled();
	}

	{
		ImGui::Separator();
		auto enableDriveVelocity = joint->IsEnableDriveVelocity();
		if (ImGui::Checkbox("Enable DriveVelocity", &enableDriveVelocity))
		{
			joint->SetEnableDriveVelocity(enableDriveVelocity);
		}

		ImGui::BeginDisabled(!enableDriveVelocity);

		auto v = joint->GetDriveVelocity();
		if (ImGui::DragFloat("Drive Velocity", &v, 0.001f, -FLT_MAX, FLT_MAX, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			joint->SetDriveVelocity(v);
		}

		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 250);
		v = joint->GetDriveForceLimit();
		if (ImGui::DragFloat("Drive Force Limit", &v, 0.000001f, 0, FLT_MAX, v == FLT_MAX ? "Infinity" : "%.6f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			joint->SetDriveForceLimit(v);
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 50);
		if (ImGui::Button(ICON_FA_ROTATE " ## reset break torque"))
		{
			joint->SetDriveForceLimit(FLT_MAX);
		}

		ImGui::EndDisabled();
	}
}

void RigidBodyInspector::InspectJointD6(Joint* _joint)
{
	static const char* MOTION_NAME[] = {
		"LOCKED",
		"LIMITED",
		"FREE"
	};

	static const ImVec4 COLOR[] = {
		ImVec4(1,0,0,1),
		ImVec4(1,1,1,1),
		ImVec4(0,1,0,1),
	};

	auto joint = (D6Joint*)_joint;

	auto AxisMotionUI = [](D6Joint* joint, D6Joint::MOTION_AXIS::ENUM axis, RigidBodyInspector* inspector)
	{
		ImGui::TextUnformatted("Motion Limit");
		auto motionType = joint->GetMotion(axis);

		//ImGui::PushID(joint + 1);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, COLOR[motionType]);
		if (ImGui::Button(MOTION_NAME[motionType], { 150,0 }))
		{
			motionType = D6Joint::MOTION_TYPE::ENUM((motionType + 1) % 3);
			joint->SetMotion(axis, motionType);
		}
		ImGui::PopStyleColor();
		//ImGui::PopID();

		ImGui::BeginDisabled(motionType != D6Joint::MOTION_TYPE::LIMITED);

		auto limit = joint->GetLinearLimit(axis);
		bool modified = inspector->InspectJointLimitBase(&limit, joint);

		if (ImGui::DragFloat("Upper", &limit.upper, 0.001f, limit.lower, FLT_MAX,
			limit.upper >= FLT_MAX / 3.0f ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		if (ImGui::DragFloat("Lower", &limit.lower, 0.001f, -FLT_MAX, limit.upper,
			limit.lower <= -FLT_MAX / 3.0f ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		if (modified)
		{
			joint->SetLinearLimit(axis, limit);
		}

		ImGui::EndDisabled();
		ImGui::Separator();
	};

	auto SwingMotionUI = [](D6Joint* joint, RigidBodyInspector* inspector)
	{
		static const char* MOTION_NAME_Y[] = {
			"LOCKED Y",
			"LIMITED Y",
			"FREE Y"
		};

		static const char* MOTION_NAME_Z[] = {
			"LOCKED Z",
			"LIMITED Z",
			"FREE Z"
		};

		ImGui::TextUnformatted("Swing Limit");
		auto motionYType = joint->GetMotion(D6Joint::MOTION_AXIS::SWING_Y);
		auto motionZType = joint->GetMotion(D6Joint::MOTION_AXIS::SWING_Z);

		//ImGui::PushID(joint + 1);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, COLOR[motionYType]);
		if (ImGui::Button(MOTION_NAME_Y[motionYType], { 150,0 }))
		{
			motionYType = D6Joint::MOTION_TYPE::ENUM((motionYType + 1) % 3);
			joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Y, motionYType);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, COLOR[motionZType]);
		if (ImGui::Button(MOTION_NAME_Z[motionZType], { 150,0 }))
		{
			motionZType = D6Joint::MOTION_TYPE::ENUM((motionZType + 1) % 3);
			joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Z, motionZType);
		}
		ImGui::PopStyleColor();
		//ImGui::PopID();

		ImGui::BeginDisabled(motionYType != D6Joint::MOTION_TYPE::LIMITED || motionZType != D6Joint::MOTION_TYPE::LIMITED);
		auto limit = joint->GetSwingLimit();
		bool modified = inspector->InspectJointLimitBase(&limit, joint);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(motionYType != D6Joint::MOTION_TYPE::LIMITED);
		if (ImGui::DragFloat("Cone Limit Y", &limit.yLimitAngle, 0.001f, 0.0f, PI,
			limit.yLimitAngle == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(motionZType != D6Joint::MOTION_TYPE::LIMITED);
		if (ImGui::DragFloat("Cone Limit Z", &limit.zLimitAngle, 0.001f, 0.0f, PI,
			limit.zLimitAngle == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}
		ImGui::EndDisabled();

		if (modified)
		{
			joint->SetSwingLimit(limit);
		}
		
		ImGui::Separator();
	};

	auto TwistMotionUI = [](D6Joint* joint, RigidBodyInspector* inspector)
	{
		ImGui::TextUnformatted("Twist Limit");
		auto motionType = joint->GetMotion(D6Joint::MOTION_AXIS::TWIST_X);

		//ImGui::PushID(joint + 1);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, COLOR[motionType]);
		if (ImGui::Button(MOTION_NAME[motionType], { 150,0 }))
		{
			motionType = D6Joint::MOTION_TYPE::ENUM((motionType + 1) % 3);
			joint->SetMotion(D6Joint::MOTION_AXIS::TWIST_X, motionType);
		}
		ImGui::PopStyleColor();
		//ImGui::PopID();

		ImGui::BeginDisabled(motionType != D6Joint::MOTION_TYPE::LIMITED);

		auto limit = joint->GetTwistLimit();
		bool modified = inspector->InspectJointLimitBase(&limit, joint);

		if (ImGui::DragFloat("Upper X Angle", &limit.upperLimit, 0.001f, limit.lowerLimit, PI / 2.0f,
			limit.lowerLimit == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		if (ImGui::DragFloat("Lower X Angle", &limit.lowerLimit, 0.001f, -PI / 2.0f, limit.upperLimit,
			limit.lowerLimit == FLT_MAX ? "Infinity" : "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		ImGui::EndDisabled();

		if (modified)
		{
			joint->SetTwistLimit(limit);
		}
		
		ImGui::Separator();
	};

	auto DriveMotionUI = [](D6Joint* joint, RigidBodyInspector* inspector, D6Joint::DRIVE_TYPE::ENUM type)
	{
		bool modified = false;

		auto drive = joint->GetDrive(type);

		if (ImGui::DragFloat("Stiffness", &drive.stiffness, 0.001f, 0.0f, FLT_MAX))
		{
			modified = true;
		}

		if (ImGui::DragFloat("Damping", &drive.damping, 0.001f, 0.0f, FLT_MAX))
		{
			modified = true;
		}

		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 250);
		if (ImGui::DragFloat("Force Limit", &drive.forceLimit, 0.000001f, 0, FLT_MAX, 
			drive.forceLimit == FLT_MAX ? "Infinity" : "%.6f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
		{
			modified = true;
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 50);
		if (ImGui::Button(ICON_FA_ROTATE " ## reset force limit"))
		{
			modified = true;
			drive.forceLimit = FLT_MAX;
		}

		if (modified)
		{
			joint->SetDrive(type, drive);
		}
	};
	
	if (ImGui::CollapsingHeader("X Axis Motion"))
	{
		AxisMotionUI(joint, D6Joint::MOTION_AXIS::X, this);
	}

	if (ImGui::CollapsingHeader("Y Axis Motion"))
	{
		AxisMotionUI(joint, D6Joint::MOTION_AXIS::Y, this);
	}

	if (ImGui::CollapsingHeader("Z Axis Motion"))
	{
		AxisMotionUI(joint, D6Joint::MOTION_AXIS::Z, this);
	}

	if (ImGui::CollapsingHeader("Swing Y,Z Motion"))
	{
		SwingMotionUI(joint, this);
	}

	if (ImGui::CollapsingHeader("Twist X Motion"))
	{
		TwistMotionUI(joint, this);
	}

	{
		ImGuiTreeNodeFlags nodeFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_Framed;
		auto open = ImGui::TreeNodeEx("Drive", nodeFlags);

		if (open)
		{
			if (ImGui::CollapsingHeader("Drive Limit X"))
			{
				DriveMotionUI(joint, this, D6Joint::DRIVE_TYPE::X);
			}

			if (ImGui::CollapsingHeader("Drive Limit Y"))
			{
				DriveMotionUI(joint, this, D6Joint::DRIVE_TYPE::Y);
			}

			if (ImGui::CollapsingHeader("Drive Limit Z"))
			{
				DriveMotionUI(joint, this, D6Joint::DRIVE_TYPE::Z);
			}

			if (ImGui::CollapsingHeader("Drive Swing"))
			{
				DriveMotionUI(joint, this, D6Joint::DRIVE_TYPE::SWING);
			}

			if (ImGui::CollapsingHeader("Drive Twist"))
			{
				DriveMotionUI(joint, this, D6Joint::DRIVE_TYPE::TWIST);
			}

			{
				bool modified = false;

				Vec3 linear, angular;
				joint->GetDriveVelocity(linear, angular);
				const char* labels[] = { "X", "Y", "Z" };

				ImGui::TextUnformatted("Linear Velocity");
				ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.88f);
				ImGui::PushID(1);
				if (ImGui::DragFloatNEx(labels, &linear[0], 3, 0.001f, -FLT_MAX, FLT_MAX))
				{
					modified = true;
				}
				ImGui::PopID();

				ImGui::TextUnformatted("Angular Velocity");
				ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.88f);
				ImGui::PushID(2);
				if (ImGui::DragFloatNEx(labels, &angular[0], 3, 0.001f, -FLT_MAX, FLT_MAX))
				{
					modified = true;
				}
				ImGui::PopID();

				if (modified)
				{
					joint->SetDriveVelocity(linear, angular);
				}
			}

			ImGui::TreePop();
		}
	}
}

void RigidBodyInspector::RenderInspectJoint()
{
	enum JOINT_TYPE {
		FIXED,
		SPHERICAL,
		REVOLUTE,
		D6
	};

	const static char* s_jointList[] = {
		"Fixed Joint",
		"Spherical Joint",
		"Revolute Joint",
		"D6 Joint",
	};

	float wHeight = 50;
	if (m_choosingJointIdx >= 0 && m_choosingJointIdx < m_body->GetJointsCount())
	{
		String typeName = m_body->GetJoint(m_choosingJointIdx)->GetClassName();
		if (typeName == "FixedJoint")
		{
			wHeight = 200;
		}
		else if (typeName == "SphericalJoint")
		{
			wHeight = 600;
		}
		else if (typeName == "RevoluteJoint")
		{
			wHeight = 700;
		}
		else if (typeName == "D6Joint")
		{
			wHeight = 800;
		}
	}

	ImGui::Dummy({ 5, 15 });
	ImGui::BeginChild("## Joint Editor", { ImGui::GetWindowWidth() * 0.88f, wHeight }, true);

	if (ImGui::Button("+ Add Joint"))
	{
		m_choosingCreateJointIdx = 0;
		m_jointCreateAnother = nullptr;

		EditorContext::DialogDesc desc;
		desc.title = "Add Joint";
		EditorContext::Get()->OpenOkCancelDialog(desc,
			[](void* p)
			{
				auto self = (RigidBodyInspector*)p;

				if (ImGui::BeginCombo("Choose Joint Type", s_jointList[self->m_choosingCreateJointIdx]))
				{
					for (size_t n = 0; n < IM_ARRAYSIZE(s_jointList); n++)
					{
						ImGui::PushID(n);
						if (ImGui::Selectable(s_jointList[n]))
						{
							self->m_choosingCreateJointIdx = int(n);
						}
						ImGui::PopID();
					}

					ImGui::EndCombo();
				}

				ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, self->m_jointCreateAnother ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1));
				ImGui::TextUnformatted(self->m_jointCreateAnother ? String::Format("Found At: [{}]", self->m_jointCreateAnother).c_str() : "Not Found");
				ImGui::PopStyleColor();
				if (ImGui::InputText("Object Name", self->m_jointCreateSearchName, sizeof(self->m_jointCreateSearchName),
					ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
				{
					self->FindJointCreateAnother();
				}

			}, this,
			[](EditorContext::DIALOG_RESULT result, void* p) -> bool
				{
					auto self = (RigidBodyInspector*)p;

					if (result == EditorContext::CANCEL)
					{
						return true;
					}

					self->FindJointCreateAnother();
					if (self->m_jointCreateAnother == nullptr)
					{
						std::cerr << "[ERROR]: no another object to create joint.\n";
						return false;
					}

					if (result == EditorContext::OK)
					{
						auto a0GlobalTransform = self->m_body->GetGameObject()->GetCommittedGlobalTransform();
						auto a1GlobalTransform = self->m_jointCreateAnother->GetGameObject()->GetCommittedGlobalTransform();

						auto temp0 = Transform::FromTransformMatrix(a0GlobalTransform);
						temp0.Scale() = { 1,1,1 };
						auto temp1 = Transform::FromTransformMatrix(a1GlobalTransform);
						temp1.Scale() = { 1,1,1 };

						auto center = (temp0.Position() + temp1.Position()) / 2.0f;
						auto jointGlobalTransform = Mat4::Translation(center);

						auto localframe0 = Transform::FromTransformMatrix(jointGlobalTransform * temp0.ToTransformMatrix().GetInverse());
						auto localframe1 = Transform::FromTransformMatrix(jointGlobalTransform * temp1.ToTransformMatrix().GetInverse());

						switch (self->m_choosingCreateJointIdx)
						{
						case JOINT_TYPE::FIXED:
							mheap::New<FixedJoint>(self->m_body, localframe0, self->m_jointCreateAnother, localframe1);
							break;
						case JOINT_TYPE::SPHERICAL:
							mheap::New<SphericalJoint>(self->m_body, localframe0, self->m_jointCreateAnother, localframe1);
							break;
						case JOINT_TYPE::REVOLUTE:
							mheap::New<RevoluteJoint>(self->m_body, localframe0, self->m_jointCreateAnother, localframe1);
							break;
						case JOINT_TYPE::D6:
							mheap::New<D6Joint>(self->m_body, localframe0, self->m_jointCreateAnother, localframe1);
							break;
						default:
							break;
						}

						self->m_countReloadJointInspectorData = 2;
					}
					return true;
				}, this
		);
	}

	if (m_body->GetJointsCount() == 0 || m_jointDatas.size() != m_body->GetJointsCount() || m_choosingJointIdx < 0)
	{
		if (m_body->GetJointsCount() != 0 && m_jointDatas.size() != 0)
		{
			OnSelectJoint(0);
		}

		ImGui::EndChild();
		return;
	}

	String previewText = String::Format("[{}] {}", m_choosingJointIdx, m_choosingJointIdx >= 0 ? m_jointDatas[m_choosingJointIdx].joint->GetClassName() : "");
	if (ImGui::BeginCombo("Choose Joint", m_choosingJointIdx >= 0 ? previewText.c_str() : nullptr))
	{
		for (size_t n = 0; n < m_jointDatas.size(); n++)
		{
			String text = String::Format("[{}] {}", n, m_jointDatas[n].joint->GetClassName());
			ImGui::PushID(n);
			if (ImGui::Selectable(text.c_str()))
			{
				m_choosingJointIdx = int(n);
				OnSelectJoint(m_choosingJointIdx);
			}
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetItemUsingMouseWheel();
		if (ImGui::GetIO().MouseWheel != 0)
		{
			int incre = ImGui::GetIO().MouseWheel < 0 ? 1 : int(m_jointDatas.size() - 1);
			m_choosingJointIdx = (m_choosingJointIdx + incre) % m_jointDatas.size();
			OnSelectJoint(m_choosingJointIdx);
		}
	}

	ImGui::Separator();

	Joint* deleteJoint = nullptr;
	auto jointsCount = m_body->GetJointsCount();
	if (m_choosingJointIdx >= 0)
	{
		auto& joint = m_body->GetJoint(m_choosingJointIdx);
		auto& jointData = m_jointDatas[m_choosingJointIdx];
		auto typeName = String(joint->GetClassName());

		//ImGui::SetNextItemOpen(true);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.f));
		auto open = ImGui::TreeNodeEx(typeName.c_str(), ImGuiTreeNodeFlags_FramePadding);
		ImGui::PopStyleVar();

		{
			auto pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 36, pos.y - 35));
			if (ImGui::Button(ICON_FA_TRASH " ## delete joint btn", ImVec2(30, 30)))
			{
				deleteJoint = joint.Get();
			}

			ImGui::SetCursorPos(pos);
		}


		if (open)
		{
			InspectJointBase(joint);

			ImGui::Dummy({ 10,10 });
			ImGui::Separator();
			//ImGui::Dummy({ 7,7 });

			{
				auto body = joint->GetBody0();
				auto count = body->GetShapesCount();
				for (size_t i = 0; i < count; i++)
				{
					DrawDebug(body, body->GetShape(i), Vec4(0, 1, 0, 1), false, true);
				}

				body = joint->GetBody1();
				count = body->GetShapesCount();
				for (size_t i = 0; i < count; i++)
				{
					DrawDebug(body, body->GetShape(i), Vec4(1, 1, 0, 1), false, true);
				}
			}

			assert(m_debugJointAnotherObject != nullptr);

			if (typeName == "FixedJoint")
			{
				InspectJointFixed(joint);
			}
			else if (typeName == "SphericalJoint")
			{
				InspectJointSpherical(joint);
			}
			else if (typeName == "RevoluteJoint")
			{
				InspectJointRevolute(joint);
			}
			else if (typeName == "D6Joint")
			{
				InspectJointD6(joint);
			}

			ImGui::TreePop();
		}
		else
		{
			if (m_debugJointAnotherObject)
			{
				if (m_debugJointAnotherObject->GetRoot() != m_body->GetGameObject()->GetRoot())
				{
					SetOpacityForObject(m_debugJointAnotherObject->GetRoot(), 1.0f);
				}
				m_debugJointAnotherObject = nullptr;
			}
		}
	}

	ImGui::EndChild();

	if (deleteJoint)
	{
		auto another = deleteJoint->GetAnotherBody(m_body);
		if (m_debugJointAnotherObject)
		{
			assert(m_debugJointAnotherObject == another->GetGameObject());
			if (m_debugJointAnotherObject->GetRoot() != m_body->GetGameObject()->GetRoot())
			{
				SetOpacityForObject(m_debugJointAnotherObject->GetRoot(), 1.0f);
			}
		}

		m_choosingJointIdx = clamp(m_choosingJointIdx, 0, int(jointsCount) - 2);
		OnSelectJoint(m_choosingJointIdx);

		deleteJoint->Break();
		m_countReloadJointInspectorData = 2;
	}
}

void RigidBodyInspector::DrawDebugImpl(const Mat4& globalTransformMat, PhysicsShape* shape, const Vec4& color, bool showBasis)
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics)
	{
		return;
	}

	Transform globalTransform = {};
	//m_body->GetGameObject()->GetCommittedGlobalTransform().Decompose(globalTransform.Scale(), globalTransform.Rotation(), globalTransform.Position());
	globalTransformMat.Decompose(globalTransform.Scale(), globalTransform.Rotation(), globalTransform.Position());

	// physics component doesn't use scale component
	globalTransform.Scale() = { 1,1,1 };

	auto localPose = shape->GetLocalTransform();
	auto globalMat = localPose.ToTransformMatrix() * globalTransform.ToTransformMatrix();

	Transform transform = {};
	globalMat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

	if (showBasis)
	{
		debugGraphics->DrawRay(globalMat.Position(), globalMat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
		debugGraphics->DrawRay(globalMat.Position(), globalMat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
		debugGraphics->DrawRay(globalMat.Position(), globalMat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });
	}

	auto type = shape->GetType();
	switch (type)
	{
	case PHYSICS_SHAPE_TYPE_SPHERE:
	{
		auto sphere = (PhysicsShapeSphere*)shape;
		debugGraphics->DrawSphere(Sphere(transform.Position(), sphere->GetRadius()), color);
		break;
	}
	case PHYSICS_SHAPE_TYPE_CAPSULE:
	{
		// physx capsule up direction forwards to x-axis
		auto capsule = (PhysicsShapeCapsule*)shape;
		debugGraphics->DrawCapsule(Capsule(globalMat.Right().Normal(), transform.Position(), capsule->GetHeight(), capsule->GetRadius()), color);
		break;
	}
	case PHYSICS_SHAPE_TYPE_BOX:
	{
		auto box = (PhysicsShapeBox*)shape;
		auto tTransform = localPose;
		tTransform.Scale() = box->GetDimensions() / 2.0f;
		auto mat = tTransform.ToTransformMatrix() * globalTransform.ToTransformMatrix();
		debugGraphics->DrawCube(mat, color);
		break;
	}
	case PHYSICS_SHAPE_TYPE_PLANE:
	{
		auto plane = (PhysicsShapePlane*)shape;
		transform.Scale().x = 0.01f;
		transform.Scale().y = 100.0f;
		transform.Scale().z = 100.0f;
		debugGraphics->DrawCube(transform.ToTransformMatrix(), color);
		break;
	}
	case PHYSICS_SHAPE_TYPE_CONVEX_MESH:
		break;
	case PHYSICS_SHAPE_TYPE_TRIANGLE_MESH:
		break;
	default:
		break;
	}
}

void RigidBodyInspector::DrawDebug(RigidBody* body, PhysicsShape* shape, const Vec4& color, bool showBasis, bool isDebugJointAnotherObject)
{
	if (body && body != m_body && isDebugJointAnotherObject)
	{
		m_debugJointAnotherObject = body->GetGameObject();
	}

	if (!body)
	{
		body = m_body;
	}

	auto& data = m_currentDrawData[shape];
	if (!data.shape)
	{
		data.shape = shape->shared_from_this();
	}

	data.globalTransformMat = body->GetGameObject()->GetCommittedGlobalTransform();
	data.color = color;
	data.showBasis = showBasis;
}

void RigidBodyInspector::FlushDrawDebug()
{
	for (auto& d : m_currentDrawData)
	{
		DrawDebugImpl(d.second.globalTransformMat, d.second.shape.get(), d.second.color, d.second.showBasis);
	}
}

void RigidBodyInspector::OnSelectShape(int idx)
{
	m_choosingShapeIdx = idx;
	m_tempShapeLocalTransform = m_shapeDatas[idx].shape->GetLocalTransform();

	m_tempIsEnableFamilyNoCollide = m_shapeDatas[idx].shape->IsEnableFamilyNoCollide();
}

void RigidBodyInspector::OnSelectJoint(int idx)
{
	m_choosingJointIdx = idx;

	auto& joint = m_body->GetJoint(idx);
	m_tempJointTransform = joint->GetGlobalTransform();
	m_tempJointTransform.Scale() = { 1,1,1 };
}

void RigidBodyInspector::ScaleBodyFromRootObject(float scaleFactor)
{
	std::set<Joint*> processedJoints;
	std::set<RigidBody*> processedBodies;

	static bool (*ProcessGameObject)(GameObject*, float, std::set<Joint*>&, std::set<RigidBody*>&) = [](GameObject* obj, float scaleFactor,
		std::set<Joint*>& processedJoints, std::set<RigidBody*>& processedBodies) -> bool
		{
			if (obj->HasComponent<RigidBody>() && processedBodies.find(obj->GetComponentRaw<RigidBody>()) == processedBodies.end())
			{
				auto body = obj->GetComponentRaw<RigidBody>();
				processedBodies.insert(body);

				body->ScaleBy(scaleFactor);
				obj->SetGlobalTransform(obj->GetCommittedGlobalTransform() * Mat4::Scaling(scaleFactor, scaleFactor, scaleFactor));

				auto count = body->GetJointsCount();
				for (size_t i = 0; i < count; i++)
				{
					auto& joint = body->GetJoint(i);
					if (processedJoints.find(joint.Get()) == processedJoints.end())
					{
						processedJoints.insert(joint.Get());

						auto l0 = joint->GetLocalFrame(joint->GetBody0());
						auto l1 = joint->GetLocalFrame(joint->GetBody1());

						joint->SetLocalFrame(joint->GetBody0(), Transform::FromTransformMatrix(l0.ToTransformMatrix() * Mat4::Scaling(Vec3(scaleFactor))));
						joint->SetLocalFrame(joint->GetBody1(), Transform::FromTransformMatrix(l1.ToTransformMatrix() * Mat4::Scaling(Vec3(scaleFactor))));

						ProcessGameObject(joint->GetAnotherBody(body)->GetGameObject(), scaleFactor, processedJoints, processedBodies);
					}
				}
			}

			return false;
		};

	auto root = m_body->GetGameObject()->GetRoot();
	root->PreTraversal(
		[&](GameObject* o) 
		{
			ProcessGameObject(o, scaleFactor, processedJoints, processedBodies);
			return false;
		}
	);
}

void RigidBodyInspector::DrawDebugShapeFromRoot()
{
	auto root = m_body->GetGameObject()->GetRoot();
	root->PreTraversal(
		[&](GameObject* obj)
		{
			if (obj->HasComponent<RigidBody>())
			{
				auto body = obj->GetComponentRaw<RigidBody>();
				auto count = body->GetShapesCount();
				for (size_t i = 0; i < count; i++)
				{
					auto shape = body->GetShape(i);
					DrawDebug(body, shape, { 1,0,0,1 }, false, false);
				}
			}

			return false;
		}
	);
}

void RigidBodyInspector::Inspect()
{
	m_currentDrawData.clear();

	bool disableAll = false;

	if (m_countReloadShapeInspectorData != 0)
	{
		if (--m_countReloadShapeInspectorData == 0)
		{
			LoadShapeInspectorDatas();
		}

		//disableAll = true;
		//return;
	}

	if (m_countReloadJointInspectorData != 0)
	{
		if (--m_countReloadJointInspectorData == 0)
		{
			LoadJointInspectorDatas();
		}

		//disableAll = true;
		//return;
	}

	ImGui::BeginDisabled(disableAll);

	{
		const char* switchStr = nullptr;
		if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			switchStr = ICON_FA_REPEAT " To Static";
		}
		else if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_STATIC)
		{
			switchStr = ICON_FA_REPEAT " To Dynamic";
		}
		else
		{
			assert(0);
		}

		if (ImGui::Button(switchStr, { 150,0 }))
		{
			Handle<RigidBody> newBody;
			Serializer serializer = {};
			auto oriShape = serializer.Clone(m_body->GetShape(0)->shared_from_this());

			if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
			{
				newBody = mheap::New<RigidBodyStatic>(oriShape);
			}
			else if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_STATIC)
			{
				newBody = mheap::New<RigidBodyDynamic>(oriShape);
			}

			auto count = m_body->GetShapesCount();
			for (size_t i = 1; i < count; i++)
			{
				newBody->AddShape(serializer.Clone(m_body->GetShape(i)->shared_from_this()));
			}

			auto obj = m_body->GetGameObject();
			obj->RemoveComponentRaw(m_body);
			obj->AddComponent(newBody);
			m_body = newBody;

			auto currentTab = EditorContext::Get()->GetCurrentTab();
			if (dynamic_cast<SceneEditorTab*>(currentTab))
			{
				dynamic_cast<SceneEditorTab*>(currentTab)->ReloadCurrentInspectingObject();
			}

			return;
		}

		if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			bool isKinematic = ((RigidBodyDynamic*)m_body)->IsKinematic();
			ImGui::SameLine(0, 10);
			if (ImGui::Checkbox("Kinematic", &isKinematic))
			{
				((RigidBodyDynamic*)m_body)->SetKinematic(isKinematic);
			}

			ImGui::SameLine(0, 10);
			if (ImGui::Checkbox("Famiily No Collide", &m_tempIsEnableFamilyNoCollide))
			{
				RigidBodyDynamic::SetFamilyNoCollideForGameObject(m_body->GetGameObject()->GetRoot(), m_tempIsEnableFamilyNoCollide);
			}
		}
	}

	{
		// adjust opacity to edit shapes
		if (ImGui::SliderFloat("Opacity", &m_currentAlpha, 0.0f, 1.0f))
		{
			auto root = m_body->GetGameObject()->GetRoot();
			SetOpacityForObject(root, m_currentAlpha);
		}
	}

	if (m_isDrawDebugAllBodiesFromRoot)
	{
		DrawDebugShapeFromRoot();
	}

	{
		float scaleOffset = 0.0f;
		if (ImGui::DragFloat("Family Scale", &scaleOffset, 0.001f, -INFINITY, INFINITY, ""))
		{
			ScaleBodyFromRootObject(1 + scaleOffset);
		}

		if (ImGui::IsItemHovered())
		{
			m_isDrawDebugAllBodiesFromRoot = true;
		}
		else
		{
			m_isDrawDebugAllBodiesFromRoot = false;
		}
	}

	if (m_bodyType == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
	{
		auto body = (RigidBodyDynamic*)m_body;
		auto v = body->GetDensity();
		auto mass = body->GetMass();
		ImGui::Text("Current Mass: %.3f", mass);
		if (ImGui::DragFloat("Body Density", &v, 0.01f, 0.01f, INFINITY))
		{
			body->SetDensity(v);
		}
	}

	RenderInspectShape();
	RenderInspectJoint();

	ImGui::EndDisabled();

	FlushDrawDebug();
}

void RigidBodyInspector::OnBeginInspecting()
{
	auto root = m_body->GetGameObject()->GetRoot();
	SetOpacityForObject(root, m_currentAlpha);

	if (m_choosingShapeIdx < 0 && m_body->GetShapesCount() != 0)
	{
		OnSelectShape(0);
	}
}

void RigidBodyInspector::OnEndInspecting()
{
	auto root = m_body->GetGameObject()->GetRoot();
	SetOpacityForObject(root, 1.0f);

	if (m_debugJointAnotherObject)
	{
		SetOpacityForObject(m_debugJointAnotherObject->GetRoot(), 1.0f);
	}
}
