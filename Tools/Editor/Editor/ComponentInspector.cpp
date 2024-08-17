#include "ComponentInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "MainSystem/Physics/Components/RigidBody.h"

#include "DataInspector.h"
#include "RigidBodyInspector.h"
#include "AnimatorInspector.h"

#include "imgui/imgui.h"

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

ComponentInspector::ComponentInspector()
{
	m_map["AnimatorSkeletalArray"] = InspectAnimatorSkeletalArray;

	m_map["RigidBodyDynamic"] = InspectRigidBody;
	m_map["RigidBodyStatic"] = InspectRigidBody;
}

String ComponentInspector::GetName(const String& name, void* comp)
{
	return String::Format("ComponentInspector_{}[{}]", name, comp);
}

void ComponentInspector::BeginInspectingFor(GameObject* obj, ClassMetadata* meta, MainComponent* comp)
{
	auto fn = [comp](const String& name, void* p)
	{
		if (name.Find("ComponentInspector_") != 0)
		{
			return;
		}

		auto compInspector = (ComponentInspectorBase*)p;
		if (comp == nullptr || comp == compInspector->m_boundComp)
		{
			compInspector->OnBeginInspecting();
		}
	};

	meta->GenericDictionary()->ForEach(fn);

	meta->ForEachSubClassProperties(
		[&, comp](ClassMetadata* meta, const char* name)
		{
			meta->GenericDictionary()->ForEach(fn);
		}
	);
}

void ComponentInspector::EndInspectingFor(GameObject* obj, ClassMetadata* meta, MainComponent* comp)
{
	auto fn = [comp](const String& name, void* p)
	{
		if (name.Find("ComponentInspector_") != 0)
		{
			return;
		}

		auto compInspector = (ComponentInspectorBase*)p;
		if (comp == nullptr || comp == compInspector->m_boundComp)
		{
			compInspector->OnEndInspecting();
		}
	};

	meta->GenericDictionary()->ForEach(fn);

	meta->ForEachSubClassProperties(
		[&, comp](ClassMetadata* meta, const char* name)
		{
			meta->GenericDictionary()->ForEach(fn);
		}
	);
}

void ComponentInspector::InspectAnimatorSkeletalArray(EditorContext* ctx, Serializable* comp, ClassMetadata* metadata, const char* propertyName)
{
	auto name = GetName("AnimatorInspector", comp);
	auto inspector = metadata->GenericDictionary()->Get<AnimatorInspector>(name);
	if (inspector == nullptr)
	{
		inspector = mheap::New<AnimatorInspector>(dynamic_cast<AnimatorSkeletalArray*>(comp), metadata);
		metadata->GenericDictionary()->Store(name, inspector);

		inspector->OnBeginInspecting();
	}

	inspector->Inspect();
}

void ComponentInspector::InspectRigidBody(EditorContext* ctx, Serializable* comp, ClassMetadata* meta, const char* propertyName)
{
	auto name = GetName("RigidBodyInspector", comp);
	auto inspector = meta->GenericDictionary()->Get<RigidBodyInspector>(name);
	if (inspector == nullptr)
	{
		inspector = mheap::New<RigidBodyInspector>(dynamic_cast<RigidBody*>(comp), meta);
		meta->GenericDictionary()->Store(name, inspector);

		inspector->OnBeginInspecting();
	}

	inspector->Inspect();
}
