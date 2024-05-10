#include "AnimatorEditorTab.h"

#include "imgui/imgui.h"

void AnimatorEditorTab::OnObjectsAdded(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnObjectsRemoved(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnRenderGUI()
{
	ImGui::Begin("Scene2");

	ImGui::Text("Hello");

	ImGui::End();
}

void AnimatorEditorTab::OnRenderInGameDebugGraphics()
{
}
