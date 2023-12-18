#include "ComponentInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "DataInspector.h"

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
}

void ComponentInspector::InspectAnimatorSkeletalArray(EditorContext* ctx, Serializable* comp, ClassMetadata* metadata, const char* propertyName)
{
	const static char* cacheNameFmt = "editor_InspectAnimatorSkeletalArray_{}";
	struct Cache
	{
		//ID animId = 0;
		//float animStartTime = 0;
		//float animEndTime = 0;

		ID blendAnimId = 0;
		float blendAnimStartTime = 0;
		float blendAnimEndTime = 0;
		float blendTime = 0;
	};

	auto cacheName = String::Format(cacheNameFmt, propertyName);

	auto cache = metadata->GenericDictionary()->Get<Cache>(cacheName);

	if (!cache)
	{
		cache = mheap::New<Cache>();
		metadata->GenericDictionary()->Store(cacheName, cache);
	}

	metadata->ForEachProperties(
		[](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			DataInspector::Inspect(metadata, accessor, propertyName);
		},
		nullptr
	);

	auto animator = (AnimatorSkeletalArray*)comp;

	if (ImGui::Button("Replay"))
	{

	}

	if (ImGui::Button("Stop"))
	{

	}

	if (ImGui::Button("Continue"))
	{

	}

	ImGui::SeparatorText("Blend Animation");

	HelpMarker("Blend to the current animation");

	bool modified = false;

	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5);

	auto& animation = animator->m_model3D->m_animations[cache->blendAnimId];
	auto duration = animation.tickDuration / animation.ticksPerSecond;

	int temp = (int)cache->blendAnimId;
	modified |= ImGui::DragInt("Blend Animation Id", (int*)&temp, 0.1f, 0, animator->m_model3D->m_animations.size() - 1);
	modified |= ImGui::DragFloat("Start Time", &cache->blendAnimStartTime, 0.01f, -INFINITY, duration);
	modified |= ImGui::DragFloat("End Time", &cache->blendAnimEndTime, 0.01f, -INFINITY, INFINITY);
	modified |= ImGui::DragFloat("Blend Duration", &cache->blendTime, 0.01f, 0, INFINITY);

	ImGui::Text("Animation name: %s", animation.name.c_str());
	ImGui::Text("Animation duration: %f sec", duration);

	ImGui::PopItemWidth();

	if (modified)
	{
		cache->blendAnimId = temp;

		cache->blendAnimEndTime = std::max(cache->blendAnimStartTime + 0.1f, cache->blendAnimEndTime);
	}
	
	if (ImGui::Button("Play"))
	{
		animator->Play(cache->blendAnimId, cache->blendAnimStartTime, cache->blendAnimEndTime, cache->blendTime);
	}
}
