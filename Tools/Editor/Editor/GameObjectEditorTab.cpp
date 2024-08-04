#include "GameObjectEditorTab.h"

#include "GameObjectEditorSaveData.h"

#include "imgui/imgui.h"

void GameObjectEditorTab::OnRenderGUI()
{
	RenderInspectorPanel();

	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	if (m_pinHierarchyPanel)
	{
		wflags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	}

	ImGui::Begin("Hierarchy", 0, wflags);

	ImGui::Checkbox("Pin Hierarchy Panel", &m_pinHierarchyPanel);

	ImGui::Separator();

	RenderHierarchyPanelGameObjectsTree(m_rootObject);

	ShowCreateGameObjectPopup();

Return:
	ImGui::End();
}

void GameObjectEditorTab::OnRenderInGameDebugGraphics()
{
	SceneEditorTab::OnRenderInGameDebugGraphics();

	EditorContext::OxyzRenderConfig config;
	config.RenderOxzGrid = false;
	config.AxisYLength = 200.0f;
	config.AxisYColor = { 1,1,1,0.5f };
	EditorContext::GetInstance()->RenderOxyz(config);
}

void GameObjectEditorTab::OnShow()
{
	m_onSaveListenerId = EditorContext::Get()->EventDispatcher()->AddListener(EditorContext::EVENT::MENU_ON_SAVE,
		[](EditorContext* ctx, int argc, void** argv, ID id)
		{
			auto self = (GameObjectEditorTab*)id;
			auto path = GetSavePath(self->m_name);

			GameObjectEditorSaveData data(self);

			Serializer serializer = {};
			serializer.Serialize(self->m_scene);
			serializer.SetRootUUID(self->m_scene->GetUUID(), 0);

			serializer.Serialize(&data);
			serializer.SetRootUUID(data.GetUUID(), 1);

			serializer.WriteToFile(path);
		},
		ID(this)
	);
}

String GameObjectEditorTab::GetSavePath(const String& name)
{
	return EditorContext::Get()->GetSavePath() + "GameObjectEditor/" + name + ".json";
}
