#include "CCTInspector.h"

#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShape.h"

#include "imgui/imgui.h"

#include "DataInspector.h"

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

	m_metadata->ForEachProperties(
		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			ImGui::Text(propertyName);
			DataInspector::Inspect(metadata, accessor, propertyName);
			return false;
		},

		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
		}
	);
}
