#include "RigidBodyInspector.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/RigidBodyStatic.h"

#include "MainSystem/Physics/Materials/PhysicsMaterial.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeSphere.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "Scene/GameObject.h"

#include "imgui/imgui.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "DataInspector.h"
#include "EditorContext.h"
#include "SceneEditorTab.h"

RigidBodyInspector::RigidBodyInspector(RigidBody* body, ClassMetadata* metadata) : ComponentInspectorBase(body), m_body(body), m_metadata(metadata)
{
	m_bodyType = m_body->GetPhysicsType();

	LoadShapeInspectorDatas();
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
}

void RigidBodyInspector::InspectShapeBase(PhysicsShape* shape)
{
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

		auto localMat = localPose.ToTransformMatrix();
		if (rotationAxis == localMat.Right().Normal())
		{
			rotationAxis = globalMat.Right().Normal();
		}

		if (rotationAxis == localMat.Up().Normal())
		{
			rotationAxis = globalMat.Up().Normal();
		}

		if (rotationAxis == localMat.Forward().Normal())
		{
			rotationAxis = globalMat.Forward().Normal();
		}

		dbGr->DrawLineSegment(
			globalMat.Position() - rotationAxis * 10.0f,
			globalMat.Position() + rotationAxis * 10.0f,
			Vec4(rotationAxis, 1.0f),
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
	
	
}

void RigidBodyInspector::InspectShapeCapsule(PhysicsShape* shape)
{
}

void RigidBodyInspector::InspectShapePlane(PhysicsShape* shape)
{
}

void RigidBodyInspector::InspectShapeSphere(PhysicsShape* shape)
{
}

void RigidBodyInspector::InspectMaterials(PhysicsShape* shape)
{
}

void RigidBodyInspector::DrawDebug(PhysicsShape* shape, const Vec4& color)
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics)
	{
		return;
	}

	Transform globalTransform = {};
	m_body->GetGameObject()->GetCommittedGlobalTransform().Decompose(globalTransform.Scale(), globalTransform.Rotation(), globalTransform.Position());

	// physics component doesn't use scale component
	globalTransform.Scale() = { 1,1,1 };

	auto localPose = shape->GetLocalTransform();
	auto globalMat = localPose.ToTransformMatrix() * globalTransform.ToTransformMatrix();

	Transform transform = {};
	globalMat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

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
		//transform.Scale().x = 0.01f;
		//transform.Scale().y = 100.0f;
		//transform.Scale().z = 100.0f;
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

void RigidBodyInspector::OnSelectShape(int idx)
{
	m_choosingShapeIdx = idx;
	m_tempShapeLocalTransform = m_shapeDatas[idx].shape->GetLocalTransform();
}

void RigidBodyInspector::SetOpacityForObject(GameObject* o, float alpha)
{
	if (o->GetComponentRaw<RenderingComponent>())
	{
		o->GetComponentRaw<RenderingComponent>()->SetOpacity(alpha);
		return;
	}

	for (auto& c : o->Children())
	{
		SetOpacityForObject(c, alpha);
	}
}

void RigidBodyInspector::Inspect()
{
	static const char* s_shapeList[] = {
		"Sphere",
		"Capsule",
		"Box",
		"Plane",
	};

	if (m_countReloadShapeInspectorData != 0)
	{
		if (--m_countReloadShapeInspectorData == 0)
		{
			LoadShapeInspectorDatas();
		}
	}

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

	ImGui::BeginChild(ID(this), {ImGui::GetWindowWidth() * 0.88f, 500}, true);

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
					auto& sampleMaterial = self->m_body->GetShape(0)->GetFirstMaterial();

					SharedPtr<PhysicsShape> shape = nullptr;
					SharedPtr<PhysicsMaterial> material = std::make_shared<PhysicsMaterial>(
						sampleMaterial->GetStaticFriction(), 
						sampleMaterial->GetDynamicFriction(),
						sampleMaterial->GetRestitution()
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

	if (ImGui::BeginCombo("Choose Shape", m_choosingShapeIdx >= 0 ? s_shapeList[m_shapeDatas[m_choosingShapeIdx].shape->GetType()] : nullptr))
	{
		for (size_t n = 0; n < m_shapeDatas.size(); n++)
		{
			ImGui::PushID(n);
			if (ImGui::Selectable(s_shapeList[m_shapeDatas[n].shape->GetType()]))
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
		if (ImGui::GetIO().MouseWheel)
		{
			int incre = ImGui::GetIO().MouseWheel > 0 ? 1 : int(m_shapeDatas.size() - 1);
			m_choosingShapeIdx = (m_choosingShapeIdx + incre) % m_shapeDatas.size();
			OnSelectShape(m_choosingShapeIdx);
		}
	}

	ImGui::Separator();

	auto shapesCount = m_body->GetShapesCount();
	if (m_choosingShapeIdx >= 0)
	{
		auto shape = m_body->GetShape(m_choosingShapeIdx);
		auto type = shape->GetType();

		auto typeName = shape->GetClassName();

		InspectMaterials(shape);

		ImGui::SetNextItemOpen(true);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.f));
		auto open = ImGui::TreeNodeEx(typeName, ImGuiTreeNodeFlags_FramePadding);
		ImGui::PopStyleVar();

		if (open)
		{
			InspectShapeBase(shape);

			DrawDebug(shape, Vec4(0, 1, 0, 1));

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

			ImGui::TreePop();
		}
	}

	for (size_t i = 0; i < shapesCount; i++)
	{
		if (i != m_choosingShapeIdx)
		{
			DrawDebug(m_body->GetShape(i), Vec4(0.8f, 0, 0, 1));
		}
	}

	ImGui::EndChild();
}

void RigidBodyInspector::OnBeginInspecting()
{
	auto root = m_body->GetGameObject()->GetRoot();
	SetOpacityForObject(root, m_currentAlpha);
}

void RigidBodyInspector::OnEndInspecting()
{
	auto root = m_body->GetGameObject()->GetRoot();
	SetOpacityForObject(root, 1.0f);
}
