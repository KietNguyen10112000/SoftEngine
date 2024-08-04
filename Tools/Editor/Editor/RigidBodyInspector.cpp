#include "RigidBodyInspector.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/RigidBodyStatic.h"

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
	auto transform = shape->GetLocalTransform();
	auto modified = transform;
	auto accessor = Accessor::For("Transform", modified, m_body);
	DataInspector::InspectTransform(m_metadata, accessor, accessor.Get(), String::Format("ShapeLocalTransform {}", shape).c_str());
	if (!transform.Equals(modified))
	{
		shape->SetLocalTransform(modified);
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

void RigidBodyInspector::DrawDebug(PhysicsShape* shape, const Vec4& color)
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics)
	{
		return;
	}

	auto localPose = shape->GetLocalTransform();
	auto globalMat = localPose.ToTransformMatrix() * m_body->GetGameObject()->GetCommittedGlobalTransform();

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
		auto capsule = (PhysicsShapeCapsule*)shape;
		debugGraphics->DrawCapsule(Capsule(globalMat.Up().Normal(), transform.Position(), capsule->GetHeight(), capsule->GetRadius()), color);
		break;
	}
	case PHYSICS_SHAPE_TYPE_BOX: 
	{
		auto box = (PhysicsShapeBox*)shape;
		debugGraphics->DrawCube(globalMat, color);
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

	ImGui::BeginChild(ID(this), {ImGui::GetWindowWidth() * 0.85f, 500}, true);

	if (ImGui::BeginCombo("Choose Shape", m_choosingShapeIdx >= 0 ? m_shapeDatas[m_choosingShapeIdx].shape->GetClassName() : nullptr))
	{
		for (size_t n = 0; n < m_shapeDatas.size(); n++)
		{
			ImGui::PushID(n);
			if (ImGui::Selectable(m_shapeDatas[m_choosingShapeIdx].shape->GetClassName()))
			{
				m_choosingShapeIdx = int(n);
			}
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
	auto shapesCount = m_body->GetShapesCount();
	if (m_choosingShapeIdx >= 0)
	{
		auto shape = m_body->GetShape(m_choosingShapeIdx);
		auto type = shape->GetType();

		auto typeName = shape->GetClassName();

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
