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
#include "EditorTabFactory.h"

#include "Resources/Resource.h"

EditorContext* EditorContext::s_instance = nullptr;

EditorContext::EditorContext(Scene* initScene)
{
	ReloadSerializableList();

	if (initScene)
	{
		auto defaultTab = mheap::New<SceneEditorTab>();
		defaultTab->m_id = 0;
		defaultTab->m_scene = initScene;
		defaultTab->m_isShowing = true;

		m_tabs.Push(defaultTab);

		m_currentTabId = 0;
		m_tabs[m_currentTabId]->Show();
	}
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

		ImGui::Separator();
		if (ImGui::MenuItem("Save"))
		{
			String path("");
			EventDispatcher()->Dispatch(EVENT::MENU_ON_SAVE, &path);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Runtime"))
	{
		if (ImGui::MenuItem("Run GC"))
		{
			gc::Run(-1);
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

	bool openCreatePopUp = false;

	if (ImGui::BeginTabBar("MyTabBar"))
	{
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Tab, { 0.5,0.5,0.5,1 });

		auto curTabId = m_currentTabId;
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

				tab->Close();

				auto scene = tab->m_scene;
				m_tabs.erase(i);
				i--;
				if (m_currentTabId != 0)
				{
					m_currentTabId = m_currentTabId - 1;
					curTabId = m_currentTabId;

					m_tabs[m_currentTabId]->Show();
				}

				{
					size_t c = 0;
					for (auto& t : m_tabs)
					{
						t->m_id = c;
						c++;
					}
				}

				Runtime::Get()->SetRunningScene(m_tabs[m_currentTabId]->m_scene);
				Runtime::Get()->DestroyScene(scene);
			}
		}

		if (curTabId != m_currentTabId)
		{
			m_tabs[curTabId]->Hide();
			m_tabs[m_currentTabId]->Show();
		}

		ImGui::PopStyleColor();

		if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
		{
			ImGui::OpenPopup("Chose");

			/*auto scene = Runtime::Get()->CreateScene();
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

			Runtime::Get()->SetRunningScene(scene);*/
		}

		if (ImGui::BeginPopup("Chose"))
		{
			EditorTabFactoryManager::Get()->ForEach(
				[&](const String& factoryName, EditorTabFactory* factory)
				{
					if (ImGui::Selectable(factory->m_tabKindName.c_str()))
					{
						m_tabFactory = factory;
						ImGui::CloseCurrentPopup();

						openCreatePopUp = true;
						m_tabFactory->Begin();
					}
				}
			);

			ImGui::EndPopup();
		}
			
		ImGui::EndTabBar();
	}

	ImGui::End();

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	if (openCreatePopUp)
	{
		ImGui::OpenPopup("Create New Editor Tab");
	}
}

void EditorContext::RenderTabCreationPopUp()
{
	if (!m_tabFactory)
	{
		return;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y / 2));
	if (!ImGui::BeginPopupModal("Create New Editor Tab", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		return;
	}

	auto tabFactory = m_tabFactory;

	ImGui::Text(tabFactory->m_tabKindName.c_str(), "");

	ImGui::Separator();

	m_tabFactory->ShowCreationInputGUI();

	ImGui::Separator();

	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 100, ImGui::GetWindowHeight() - 40));

	if (ImGui::Button("OK", ImVec2(100, 0)))
	{
		auto tab = m_tabFactory->CreateInstance();

		if (!tab)
		{
			std::cerr << "TabCreation ERROR. \n";
		}
		else
		{
			/*tab->m_id = m_tabs.size();
			m_tabs.Push(tab);

			auto scene = tab->m_scene;

			auto tabId = scene->GenericStorage()->Store(tab);
			scene->EventDispatcher()->AddListener(Scene::EVENT_BEGIN_RUNNING,
				[](Scene* scene, int argc, void** argv, ID id)
				{
					auto tab = scene->GenericStorage()->Get<AnimatorEditorTab>(id);
					EditorContext::GetInstance()->m_currentTabId = tab->m_id;
					tab->Show();
				},
				tabId
			);

			Runtime::Get()->SetRunningScene(scene);*/

			RunTab(tab);
		}

		m_tabFactory = nullptr;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine(0, 20);
	if (ImGui::Button("Cancel", ImVec2(100, 0)))
	{
		m_tabFactory = nullptr;
		ImGui::CloseCurrentPopup();
	}

	if (!m_tabFactory)
	{
		tabFactory->End();
	}

	ImGui::EndPopup();
}

void EditorContext::RenderOxyz(OxyzRenderConfig& config)
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics) return;

	if (config.AxisXLength != 0.0f)
	{
		debugGraphics->DrawDirection(Vec3(-config.AxisXLength / 2.0f, 0, 0), Vec3(config.AxisXLength, 0, 0), { 1,0,0,1 }, { 1,0,0,1 });
	}

	if (config.AxisYLength != 0.0f)
	{
		debugGraphics->DrawDirection(Vec3(0, -config.AxisYLength / 2.0f, 0), Vec3(0, config.AxisYLength, 0), { 0,1,0,1 }, { 0,1,0,1 });
	}

	if (config.AxisZLength != 0.0f)
	{
		debugGraphics->DrawDirection(Vec3(0, 0, -config.AxisZLength / 2.0f), Vec3(0, 0, config.AxisZLength), { 0,0,1,1 }, { 0,0,1,1 });
	}

	if (config.RenderOxzGrid)
	{
		auto minZ = std::min(config.OxzRangeStart.y, config.OxzRangeEnd.y);
		auto maxZ = std::max(config.OxzRangeStart.y, config.OxzRangeEnd.y);
		auto lenZ = (maxZ - minZ) / 2.0f;

		auto minX = std::min(config.OxzRangeStart.x, config.OxzRangeEnd.x);
		auto maxX = std::max(config.OxzRangeStart.x, config.OxzRangeEnd.x);
		auto lenX = (maxX - minX) / 2.0f;

		Vec4 color = { 1.0f,1.0f,1.0f,0.4f };

		for (float z = minZ; z <= maxZ; z += config.OxzRangeStep)
		{
			debugGraphics->DrawLineSegment({ -lenX,0,z }, { lenX,0,z }, color, config.OxzGridThickness);
		}

		for (int x = minX; x <= maxX; x += config.OxzRangeStep)
		{
			debugGraphics->DrawLineSegment({ x,0,-lenZ }, { x,0,lenZ }, color, config.OxzGridThickness);
		}
	}

	if (config.RenderOxzPlane)
	{
		debugGraphics->DrawAABox(AABox({ 0,0,0 }, { 1000,0.5,1000 }), { 0.5,0.5,0.5,0.3 }, true);
	}
}

void EditorContext::OnObjectsAdded(std::vector<GameObject*>& objects, Scene* scene)
{
	if (scene != GetCurrentTab()->m_scene)
	{
		return;
	}

	GetCurrentTab()->OnObjectsAdded(objects);
}

void EditorContext::OnObjectsRemoved(std::vector<GameObject*>& objects, Scene* scene)
{
	if (scene != GetCurrentTab()->m_scene)
	{
		return;
	}

	GetCurrentTab()->OnObjectsRemoved(objects);
}

void EditorContext::OnRenderGUI()
{
	GetCurrentTab()->OnRenderGUI();

	RenderMenuBar();
	RenderTabBar();

	//ImGui::SetNextWindowFocus();
	//ImGui::ShowDemoWindow(0);

	RenderTabCreationPopUp();

	/*ImGui::Begin("Debug");
	if (ImGui::Button("Run GC"))
	{
		gc::Run(-1);
	}
	ImGui::End();*/

	{
		std::map<String, ResourceBase*>& map = *resource::internal::GetInternalRCMap();
		int x = 3;
	}
}

void EditorContext::OnRenderInGameDebugGraphics()
{
	GetCurrentTab()->OnRenderInGameDebugGraphics();
}

void EditorContext::OnFinalize()
{
	for (auto& tab : m_tabs)
	{
		tab->OnClose();
	}
	m_tabs.clear();
}

void EditorContext::RunTab(const Handle<EditorTab>& tab)
{
	auto scene = tab->m_scene;
	if (tab->m_id == INVALID_ID)
	{
		tab->m_id = m_tabs.size();
		m_tabs.Push(tab);

		auto tabId = scene->GenericStorage()->Store(tab);
		scene->EventDispatcher()->AddListener(Scene::EVENT_BEGIN_RUNNING,
			[](Scene* scene, int argc, void** argv, ID id)
			{
				auto tab = scene->GenericStorage()->Get<AnimatorEditorTab>(id);
				EditorContext::GetInstance()->m_currentTabId = tab->m_id;
				tab->Show();
			},
			tabId
		);
	}
	else
	{
		m_currentTabId = tab->m_id;
	}

	Runtime::Get()->SetRunningScene(scene);
}

void EditorContext::CloseTab(const Handle<EditorTab>& tab)
{
	if (m_tabs.size() <= 1)
	{
		return;
	}

	auto i = tab->m_id;

	tab->Close();

	auto scene = tab->m_scene;
	m_tabs.erase(i);
	if (m_currentTabId != 0)
	{
		m_currentTabId = m_currentTabId - 1;
		m_tabs[m_currentTabId]->Show();
	}

	{
		size_t c = 0;
		for (auto& t : m_tabs)
		{
			t->m_id = c;
			c++;
		}
	}

	Runtime::Get()->SetRunningScene(m_tabs[m_currentTabId]->m_scene);
	Runtime::Get()->DestroyScene(scene);
}
