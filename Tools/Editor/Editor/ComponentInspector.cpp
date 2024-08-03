#include "ComponentInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "MainSystem/Physics/Components/RigidBody.h"

#include "DataInspector.h"
#include "RigidBodyInspector.h"

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
	//const static char* cacheNameFmt = "editor_InspectAnimatorSkeletalArray_{}";
	//struct Cache
	//{
	//	/*ID curAnimId = 0;
	//	float curAnimStartTime = 0;
	//	float curAnimEndTime = 0;*/

	//	ID blendAnimId = 0;
	//	float blendAnimStartTime = 0;
	//	float blendAnimEndTime = 0;
	//	float blendTime = 0;
	//	float startTransitTime = -1;

	//	float curT = 0;

	//	bool isPaused = false;
	//	bool needRepause = false;
	//};

	//auto cacheName = String::Format(cacheNameFmt, propertyName);

	//auto cache = metadata->GenericDictionary()->Get<Cache>(cacheName);

	//if (!cache)
	//{
	//	cache = mheap::New<Cache>();
	//	metadata->GenericDictionary()->Store(cacheName, cache);
	//}

	//metadata->ForEachProperties(
	//	[](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
	//	{
	//		DataInspector::Inspect(metadata, accessor, propertyName);
	//	},
	//	nullptr
	//);

	//auto animator = (AnimatorSkeletalArray*)comp;

	//auto& curAnimationTrack = animator->m_currentAnimTrack;

	//if (ImGui::Button("Stop"))
	//{
	//	animator->SetPause(true);
	//	cache->isPaused = true;
	//}

	//if (ImGui::Button("Continue"))
	//{
	//	animator->SetPause(false);
	//	cache->isPaused = false;
	//}

	///*if (cache->needRepause)
	//{
	//	animator->SetPause(true);
	//	cache->isPaused = true;
	//	cache->needRepause = false;
	//}*/

	//auto curAnimBeginSec = curAnimationTrack->startTick / curAnimationTrack->ticksPerSecond;
	//auto curAnimEndSec = curAnimBeginSec + curAnimationTrack->tickDuration / curAnimationTrack->ticksPerSecond;
	//auto tempT = animator->m_t / curAnimationTrack->ticksPerSecond;
	//if (ImGui::SliderFloat("Animation Time", &tempT, curAnimBeginSec, curAnimEndSec) && cache->isPaused)
	//{
	//	//animator->SetPause(false);
	//	//cache->needRepause = true;
	//	animator->SetTime(tempT);
	//}

	//ImGui::SeparatorText("Blend Animation");

	//HelpMarker("Blend to the current animation");

	//bool modified = false;

	//ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5);

	//auto& animation = animator->m_model3D->m_animations[cache->blendAnimId];
	//auto duration = animation.tickDuration / animation.ticksPerSecond;

	//int temp = (int)cache->blendAnimId;
	//modified |= ImGui::DragInt("Blend Animation Id", (int*)&temp, 0.1f, 0, animator->m_model3D->m_animations.size() - 1);
	//modified |= ImGui::DragFloat("Begin Time", &cache->blendAnimStartTime, 0.01f, -INFINITY, duration);
	//modified |= ImGui::DragFloat("End Time", &cache->blendAnimEndTime, 0.01f, -INFINITY, INFINITY);
	//modified |= ImGui::DragFloat("Blend Duration", &cache->blendTime, 0.01f, 0, INFINITY);

	//modified |= ImGui::DragFloat("Start transit time", &cache->startTransitTime, 0.01f, 0, INFINITY);

	//ImGui::Text("Animation name: %s", animation.name.c_str());
	//ImGui::Text("Animation duration: %f sec", duration);

	//ImGui::PopItemWidth();

	//if (modified)
	//{
	//	cache->blendAnimId = temp;

	//	cache->blendAnimEndTime = std::max(cache->blendAnimStartTime + 0.1f, cache->blendAnimEndTime);
	//}
	//
	//if (ImGui::Button("Play"))
	//{
	//	animator->Play(cache->startTransitTime, cache->blendAnimId, 0, cache->blendAnimStartTime, cache->blendAnimEndTime, cache->blendTime);
	//}
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
