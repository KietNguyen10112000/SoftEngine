#include "CCTInspector.h"

#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShape.h"

#include "imgui/imgui.h"

using namespace soft;

CCTInspector::CCTInspector(CharacterController* cct, ClassMetadata* metadata)
	: RigidBodyInspector(cct, metadata)
{
}

void CCTInspector::Inspect()
{
	if (m_body->GetShapesCount() != 0)
	{
		auto mask = m_body->GetShape(0)->GetCollisionMask();
		ImGui::Separator();
		if (RenderCollisionMaskChooser(&mask, "All Collision Masks"))
		{
			m_body->SetCollisionMaskForAllShapes(mask);
		}
		ImGui::Separator();
	}
}
