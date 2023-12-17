#include "ComponentInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "DataInspector.h"

#include "imgui/imgui.h"

ComponentInspector::ComponentInspector()
{
	m_map["AnimatorSkeletalArray"] = InspectAnimatorSkeletalArray;
}

void ComponentInspector::InspectAnimatorSkeletalArray(EditorContext* ctx, Serializable* comp, ClassMetadata* metadata)
{
	metadata->ForEachProperties(
		[](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			DataInspector::Inspect(metadata, accessor, propertyName);
		},
		nullptr
	);
}
