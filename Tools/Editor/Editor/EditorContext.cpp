#include "EditorContext.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "DataInspector.h"

#include "Scene/Scene.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"
#include "MainSystem/Scripting/Components/Script.h"
#include "MainSystem/Physics/Components/PhysicsComponent.h"
#include "MainSystem/Animation/Components/AnimationComponent.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "ComponentInspector.h"

#include "SceneEditorTab.h"
#include "AnimatorEditorTab.h"

ID EditorContext::s_id = INVALID_ID;

EditorContext::EditorContext(Scene* initScene)
{
	ReloadSerializableList();

	auto defaultTab = mheap::New<SceneEditorTab>();
	defaultTab->m_scene = initScene;

	m_tabs.Push(defaultTab);

	m_currentTabId = 0;
}

void EditorContext::RenderMenuBar()
{
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Reload"))
		{
			ReloadSerializableList();
		}

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EditorContext::ReloadSerializableList()
{
	for (auto& v : m_components)
	{
		v.clear();
	}

	SerializableDB::Get()->ForEachSerializableRecord(
		[&](const SerializableDB::SerializableRecord& record)
		{
			if (record.COMPONENT_ID == INVALID_ID)
			{
				return;
			}

			m_components[record.COMPONENT_ID].push_back((SerializableDB::SerializableRecord*)&record);
		}
	);
}

void EditorContext::RenderTabBar()
{
	auto viewport = (ImGuiViewportP*)ImGui::GetMainViewport();
	auto workRect = viewport->GetBuildWorkRect();

	ImGui::SetNextWindowPos({ 0,workRect.Min.y });
	ImGui::SetNextWindowSize({ viewport->WorkSize.x,35 });
	ImGui::SetNextWindowScroll({ -1,0 });

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, { 5.0f, 3.0f });

	ImGui::Begin("RenderTabBar", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

	auto SelectableColor = [](ImU32 color)
	{
		ImVec2 p_min = ImGui::GetItemRectMin();
		ImVec2 p_max = ImGui::GetItemRectMax();
		ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, color);
	};

	struct rgba
	{
		char r;
		char g;
		char b;
		char a;

		rgba(int r, int g, int b, int a) : r(r), g(g), b(b), a(a) {};

		uint32_t ToUint32()
		{
			return *(uint32_t*)this;
		}
	};

	if (ImGui::BeginTabBar("MyTabBar"))
	{
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Tab, { 0.5,0.5,0.5,1 });

		for (size_t i = 0; i < m_tabs.size(); i++)
		{
			auto selected = i == m_currentTabId;
			bool open = true;
			auto& tab = m_tabs[i];

			ImGui::PushID(i);
			if (ImGui::BeginTabItem(tab->m_name.c_str(), &open, selected ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
			{
				ImGui::EndTabItem();
			}
			ImGui::PopID();

			if (ImGui::IsItemClicked())
			{
				m_currentTabId = i;
				Runtime::Get()->SetRunningScene(tab->m_scene);
			}

			if (!open && m_tabs.size() > 1)
			{
				// close this tab
				//m_currentTabId = i;
			}
		}

		ImGui::PopStyleColor();

		if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
		{
			auto scene = Runtime::Get()->CreateScene();
			auto tab = mheap::New<AnimatorEditorTab>();
			tab->m_scene = scene;
			tab->m_name = "Scene2";
			tab->m_id = m_tabs.size();
			m_tabs.Push(tab);

			auto tabId = scene->GenericStorage()->Store(tab);
			scene->EventDispatcher()->AddListener(Scene::EVENT_BEGIN_RUNNING,
				[](Scene* scene, int argc, void** argv, ID id)
				{
					auto tab = scene->GenericStorage()->Get<AnimatorEditorTab>(id);
					EditorContext::GetInstance()->m_currentTabId = tab->m_id;
				},
				tabId
			);

			Runtime::Get()->SetRunningScene(scene);
		}
			
		ImGui::EndTabBar();
	}

	ImGui::End();

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}

void EditorContext::OnObjectsAdded(std::vector<GameObject*>& objects)
{
	GetCurrentTab()->OnObjectsAdded(objects);
}

void EditorContext::OnObjectsRemoved(std::vector<GameObject*>& objects)
{
	GetCurrentTab()->OnObjectsRemoved(objects);
}

void EditorContext::OnRenderGUI()
{
	GetCurrentTab()->OnRenderGUI();

	RenderMenuBar();
	RenderTabBar();
	ImGui::ShowDemoWindow(0);
}

void EditorContext::OnRenderInGameDebugGraphics()
{
	GetCurrentTab()->OnRenderInGameDebugGraphics();
}
