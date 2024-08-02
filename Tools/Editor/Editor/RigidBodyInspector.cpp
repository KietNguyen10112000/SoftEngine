#include "RigidBodyInspector.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/RigidBodyStatic.h"

#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeSphere.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"

#include "Scene/GameObject.h"

#include "imgui/imgui.h"

#include "DataInspector.h"

RigidBodyInspector::RigidBodyInspector(RigidBody* body, ClassMetadata* metadata) : m_body(body), m_metadata(metadata)
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
	auto accessor = Accessor::For("Transform", transform, m_body);
	DataInspector::InspectTransform(m_metadata, accessor, accessor.Get(), String::Format("ShapeLocalTransform {}", shape).c_str());
}

void RigidBodyInspector::InspectShapeBox(PhysicsShape* shape)
{
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

void RigidBodyInspector::Inspect()
{
	{
		// adjust opacity to edit shapes
		RenderingComponent* rootRenderingComp = nullptr;
		auto it = m_body->GetGameObject();
		while (it)
		{
			auto comp = it->GetComponentRaw<RenderingComponent>();
			if (comp)
			{
				rootRenderingComp = comp;
			}
			it = it->Parent().Get();
		}

		if (rootRenderingComp)
		{
			float opacity = rootRenderingComp->GetOpacity();
			if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f))
			{
				rootRenderingComp->SetOpacity(opacity);
			}
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

			switch (type)
			{
			case soft::PHYSICS_SHAPE_TYPE_SPHERE:
				InspectShapeSphere(shape);
				break;
			case soft::PHYSICS_SHAPE_TYPE_CAPSULE:
				InspectShapeCapsule(shape);
				break;
			case soft::PHYSICS_SHAPE_TYPE_BOX:
				InspectShapeBox(shape);
				break;
			case soft::PHYSICS_SHAPE_TYPE_PLANE:
				InspectShapePlane(shape);
				break;
			case soft::PHYSICS_SHAPE_TYPE_CONVEX_MESH:
				break;
			case soft::PHYSICS_SHAPE_TYPE_TRIANGLE_MESH:
				break;
			default:
				break;
			}

			ImGui::TreePop();
		}
	}

	ImGui::EndChild();
}
