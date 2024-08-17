#include "AnimatorInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

#include "imgui/imgui.h"

AnimatorInspector::AnimatorInspector(AnimatorSkeletalArray* animator, ClassMetadata* meta) 
	: ComponentInspectorBase(animator), m_animator(animator), m_metadata(meta)
{

}

void AnimatorInspector::OnBeginInspecting()
{
	auto root = m_animator->GetGameObject()->GetRoot();
	SetOpacityForObject(root, m_currentAlpha);
}

void AnimatorInspector::OnEndInspecting()
{
	auto root = m_animator->GetGameObject()->GetRoot();
	SetOpacityForObject(root, 1.0f);
}

void AnimatorInspector::Inspect()
{
	{
		// adjust opacity to edit shapes
		if (ImGui::SliderFloat("Opacity", &m_currentAlpha, 0.0f, 1.0f))
		{
			auto root = m_animator->GetGameObject()->GetRoot();
			SetOpacityForObject(root, m_currentAlpha);
		}
	}


}