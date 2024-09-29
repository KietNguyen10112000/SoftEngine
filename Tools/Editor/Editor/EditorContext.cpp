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
#include "SystemDialog.h"

#include "Resources/Resource.h"
#include "Input/Input.h"
#include "EditorSettings.h"

EditorContext* EditorContext::s_instance = nullptr;

EditorContext::EditorContext(Scene* initScene)
{
	EditorContext::s_instance = this;

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
	static char textBuf[256] = {};

	auto currentTab = GetCurrentTab();

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
			auto currentTab = GetCurrentTab();
			if (currentTab)
			{
				String savePath = currentTab->GetSaveFilePath();
				if (savePath.empty() || !IsVariableNameValid(currentTab->m_name))
				{
					if (savePath.empty())
					{
						savePath = EditorContext::Get()->GetSavePath() + currentTab->GetTabClassName() + "/" + currentTab->m_name + "." + currentTab->GetTabClassName();
					}
					
					SystemDialog::SaveAsDialog otp;
					otp.defaultPath = FileSystem::Get()->GetCurrentWorkingDirectory() + savePath;
					otp.extensionGroups = { 
						{ 
							currentTab->GetTabClassName(), 
							{ currentTab->GetTabClassName() }
						} 
					};
					if (SystemDialog::OpenSaveAsDialog(otp))
					{
						savePath = otp.outputFilePath;
						m_savingPath = savePath;
						if (FileSystem::Get()->IsFileExisted(savePath.c_str()))
						{
							OpenOkCancelDialog({},
								[](void* p)
								{
									auto self = (EditorContext*)p;
									ImGui::TextUnformatted(String::Format("File \"{}\" existed. Override it???", self->m_savingPath).c_str());
								}, this,
								[](EditorContext::DIALOG_RESULT result, void* p) -> bool
								{
									auto self = (EditorContext*)p;

									if (result == EditorContext::DIALOG_RESULT::OK)
									{
										self->DoSave(self->m_savingPath);
									}

									return true;
								}, this
							);
						}
						else
						{
							DoSave(savePath);
						}
						
					}
				}
				else
				{
					String path("");
					FileSystem::Get()->SaveCache();
					resource::internal::SaveCache();
					EventDispatcher()->Dispatch(EVENT::MENU_ON_SAVE, &path);
				}
			}
		}

		if (currentTab)
		{
			currentTab->OnRenderMenuBar("File");
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Runtime"))
	{
		if (ImGui::MenuItem("Run GC"))
		{
			gc::Run(-1);
		}

		if (ImGui::MenuItem("Reload Script"))
		{
			Runtime::Get()->HotReloadScripts();
		}

		if (currentTab)
		{
			currentTab->OnRenderMenuBar("Runtime");
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Editor"))
	{
		if (ImGui::MenuItem("Settings"))
		{
			OpenOkCancelDialog({ "Editor Settings" },
				[](void* p)
				{
					EditorSettings::Get()->Render();
				}, this,
				[](EditorContext::DIALOG_RESULT result, void* p) -> bool
				{
					if (result == EditorContext::DIALOG_RESULT::OK)
					{
						EditorSettings::Get()->OnApplySetting();
						EditorContext::Get()->SaveConfig();
					}

					return true;
				}, this
			);
		}

		ImGui::EndMenu();
	}

	if (currentTab)
	{
		if (ImGui::BeginMenu("Tab"))
		{
			currentTab->OnRenderMenuBar("Tab");
			ImGui::EndMenu();
		}
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

			if (!open && m_tabs.size() > 1 && tab->IsCloseable())
			{
				// close this tab
				//m_currentTabId = i;

				tab->Close();

				if (m_currentTabId != 0)
				{
					if (m_currentTabId >= i)
					{
						m_currentTabId = m_currentTabId - 1;
					}
				}
				else
				{
					if (0 == i)
					{
						m_tabs[0]->Show();
					}
				}

				if (curTabId != m_currentTabId && curTabId == i)
				{
					m_tabs[m_currentTabId]->Show();
				}

				auto scene = tab->m_scene;
				m_tabs.Remove(m_tabs.begin() + i);
				i--;

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

		/*if (curTabId != m_currentTabId)
		{
			m_tabs[curTabId]->Hide();
			m_tabs[m_currentTabId]->Show();
		}*/

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
						m_tabFactory->m_overwriteExist = false;
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
	if (m_needOpennTabCreationPopUp)
	{
		ImGui::OpenPopup("Create New Editor Tab");
		m_needOpennTabCreationPopUp = false;
	}

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

	//if (m_needCloseTabCreationPopUp)
	//{
	//	m_needCloseTabCreationPopUp = false;

	//	m_tabHolder = nullptr;
	//	auto tab = m_tabFactory->CreateInstance();

	//	if (!tab)
	//	{
	//		std::cerr << "TabCreation ERROR. \n";
	//	}
	//	else
	//	{
	//		RunTab(tab);
	//		m_tabFactory = nullptr;
	//		ImGui::CloseCurrentPopup();
	//	}

	//	//ImGui::CloseCurrentPopup();
	//	ImGui::EndPopup();
	//	return;
	//}

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

void EditorContext::RenderDialogs()
{
	if (!m_closeDialogs.empty())
	{
		for (auto& d : m_closeDialogs)
		{
			CloseDialogImpl(d);
		}
		m_closeDialogs.clear();
	}

	bool close = false;
	/*if (!m_dialogs.empty())
	{
		auto& dialog = *m_dialogs.back().get();
		if (dialog.popUpId.empty())
		{
			dialog.popUpId = String::Format("{} {} {}", dialog.desc.title, "## dialog", m_dialogs.back().get());
		}
		ImGui::SetWindowFocus(dialog.popUpId.c_str());
	}*/

	//for (auto& pdialog : m_dialogs)
	bool overlayFocused = false;
	if (!m_dialogs.empty())
	{
		float w = Graphics::Get()->GetWindowWidth();
		float h = Graphics::Get()->GetWindowHeight();
		//ImGui::SetNextWindowFocus();
		ImGui::SetNextWindowSize({ w * 2.0f, h * 2.0f });
		ImGui::SetNextWindowPos({ -float(w) * 0.5f, -float(h) * 0.5f });

		w = w * 2.0f;
		h = h * 2.0f;

		//ImGui::SetNextWindowSize({ w, h });
		//ImGui::SetNextWindowPos({ 0, 0 });

		ImGui::SetNextWindowBgAlpha(0.5f);

		ImGui::Begin("Overlay ## dialog", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (ImGui::IsWindowFocused())
		{
			overlayFocused = true;
		}
		
		ImGui::End();
	}

	if (!m_dialogs.empty())
	{
		auto& pdialog = m_dialogs.back();
		//auto& dialog = *pdialog;
		auto& dialog = *m_dialogs.back().get();

		if (dialog.popUpId.empty())
		{
			dialog.popUpId = String::Format("{} {} {}", dialog.desc.title, "## dialog", pdialog.get());
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		//ImGui::SetNextWindowPos({0,0});
		if (dialog.desc.size.x <= 1.0f && dialog.desc.size.y <= 1.0f)
		{
			auto viewPortSize = ImGui::GetMainViewport()->Size;
			ImGui::SetNextWindowSize(ImVec2(viewPortSize.x * dialog.desc.size.x, viewPortSize.y * dialog.desc.size.y));
		}
		else
		{
			ImGui::SetNextWindowSize(ImVec2(dialog.desc.size.x, dialog.desc.size.y));
		}

		if (overlayFocused)
		{
			ImGui::SetNextWindowFocus();
		}
		
		ImGui::Begin(dialog.popUpId.c_str(), 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
		{
			dialog.bodyCallback(dialog.bodyUserPtr);

			ImGui::Separator();

			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 100, ImGui::GetWindowHeight() - 40));

			if (ImGui::Button("OK", ImVec2(100, 0)))
			{
				if (dialog.resultCallback(DIALOG_RESULT::OK, dialog.resultUserPtr))
				{
					close = true;
					assert(&pdialog - m_dialogs.data() == m_dialogs.size() - 1);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::SameLine(0, 20);
			if (ImGui::Button("Cancel", ImVec2(100, 0)))
			{
				if (dialog.resultCallback(DIALOG_RESULT::CANCEL, dialog.resultUserPtr))
				{
					close = true;
					assert(&pdialog - m_dialogs.data() == m_dialogs.size() - 1);
					ImGui::CloseCurrentPopup();
				}
			}

			//ImGui::EndPopup();
		}
		ImGui::End();
	}

	if (close)
	{
		m_dialogs.pop_back();
	}
}

void EditorContext::CloseDialogImpl(DialogData* dialog)
{
	ID idx = INVALID_ID;
	for (auto& d : m_dialogs)
	{
		if (d.get() == dialog)
		{
			idx = &d - m_dialogs.data();
			break;
		}
	}

	if (idx != INVALID_ID)
	{
		m_dialogs.erase(m_dialogs.begin() + idx);
	}
}

void EditorContext::DoSave(const String& path)
{
	auto currentTab = GetCurrentTab();

	auto fileWithExtension = FileUtils::GetLastName(path.c_str());
	auto fileName = fileWithExtension.SubString(0, fileWithExtension.RFind('.'));
	if (!IsVariableNameValid(fileName))
	{
		std::cerr << "[ERROR]: invalid file name!";
	}
	else
	{
		currentTab->m_name = fileName;
		currentTab->m_saveDirectory = FileUtils::PopPath(path);

		String path("");
		FileSystem::Get()->SaveCache();
		EventDispatcher()->Dispatch(EVENT::MENU_ON_SAVE, &path);
	}
}

void EditorContext::RenderOxyz(OxyzRenderConfig& config)
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics) return;

	if (config.AxisXLength != 0.0f)
	{
		debugGraphics->DrawRay(Vec3(-config.AxisXLength / 2.0f, 0, 0), Vec3(config.AxisXLength, 0, 0), config.AxisXColor, config.AxisXColor);
	}

	if (config.AxisYLength != 0.0f)
	{
		debugGraphics->DrawRay(Vec3(0, -config.AxisYLength / 2.0f, 0), Vec3(0, config.AxisYLength, 0), config.AxisYColor, config.AxisYColor);
	}

	if (config.AxisZLength != 0.0f)
	{
		debugGraphics->DrawRay(Vec3(0, 0, -config.AxisZLength / 2.0f), Vec3(0, 0, config.AxisZLength), config.AxisZColor, config.AxisZColor);
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
		for (auto& tab : m_tabs)
		{
			if (tab->m_scene == scene)
			{
				tab->OnObjectsAdded(objects);
				return;
			}
		}
		return;
	}

	GetCurrentTab()->OnObjectsAdded(objects);
}

void EditorContext::OnObjectsRemoved(std::vector<GameObject*>& objects, Scene* scene)
{
	if (scene != GetCurrentTab()->m_scene)
	{
		for (auto& tab : m_tabs)
		{
			if (tab->m_scene == scene)
			{
				tab->OnObjectsRemoved(objects);
				return;
			}
		}
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
	ImGui::ShowDemoWindow(0);

	RenderTabCreationPopUp();

	RenderDialogs();

	/*ImGui::Begin("Debug");
	if (ImGui::Button("Run GC"))
	{
		gc::Run(-1);
	}
	ImGui::End();*/

	/*{
		std::map<String, ResourceBase*>& map = *resource::internal::GetInternalRCMap();
		int x = 3;
	}*/
}

void EditorContext::OnRenderInGameDebugGraphics()
{
	GetCurrentTab()->OnRenderInGameDebugGraphics();
}

void EditorContext::OnFinalize()
{
	for (auto& tab : m_tabs)
	{
		tab->Close();
	}
	m_tabs.clear();
}

void EditorContext::RunTab(const Handle<EditorTab>& tab)
{
	auto scene = tab->m_scene;
	if (tab->m_id == INVALID_ID)
	{
		if (tab->m_isPlacedHolder == false)
		{
			tab->m_id = m_tabs.size();
			m_tabs.Push(tab);
			tab->m_isPlacedHolder = true;
		}
		else
		{
			tab->m_id = m_tabs.size() - 1;
		}

		auto tabId = scene->GenericStorage()->Store(tab);
		scene->EventDispatcher()->AddListener(Scene::EVENT_BEGIN_RUNNING,
			[](Scene* scene, int argc, void** argv, ID id)
			{
				auto tab = scene->GenericStorage()->Get<EditorTab>(id);

				if (EditorContext::GetInstance()->m_currentTabId != tab->m_id)
				{
					EditorContext::GetInstance()->GetCurrentTab()->Hide();
					EditorContext::GetInstance()->m_currentTabId = tab->m_id;
					tab->Show();
				}
			},
			tabId
		);
	}
	/*else
	{
		m_currentTabId = tab->m_id;
	}*/

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
	m_tabs.Remove(m_tabs.begin() + i);
	if (m_currentTabId != 0)
	{
		m_currentTabId = m_currentTabId - 1;
	}

	m_tabs[m_currentTabId]->Show();

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

bool EditorContext::IsVariableNameValid(const String& name)
{
	std::string_view view = name.c_str();
	if (view.empty()
		|| std::isdigit(view[0]))
	{
		std::cerr << "[ERROR]: Invalid Variable's name!\n";
		return false;
	}

	bool ret = true;
	std::for_each(view.begin(), view.end(),
		[&](char c)
		{
			if (!std::isalnum(c))
			{
				if (c != '_')
				{
					std::cerr << "[ERROR]: Variable's name can not be contained special characters!\n";
					ret = false;
					return;
				}
			}
		}
	);

	return ret;
}

EditorContext::DialogData* EditorContext::OpenOkCancelDialog(const DialogDesc& desc, DialogBodyCallback bodyCallback, void* bodyUserPtr, DialogResultCallback resultCallback, void* resultUserPtr)
{
	auto ptr = std::make_unique<DialogData>(
		bodyCallback, bodyUserPtr,
		resultCallback, resultUserPtr,
		desc
	);
	auto ret = ptr.get();
	m_dialogs.push_back(std::move(ptr));

	return ret;
}

void EditorContext::CloseDialog(DialogData* dialog)
{
	m_closeDialogs.push_back(dialog);
}

void EditorContext::OpenTabCreationPopUp()
{
	m_needOpennTabCreationPopUp = true;
}

void EditorContext::CloseTabCreationPopUp()
{
	m_needCloseTabCreationPopUp = true;
}

void EditorContext::PlaceHolderTab(EditorTab* tab)
{
	if (tab == (EditorTab*)INVALID_ID)
	{
		m_tabHolder = nullptr;
		m_tabs.Pop();
		return;
	}

	if (tab == nullptr)
	{
		m_tabHolder = nullptr;
		return;
	}

	if (!tab->m_isPlacedHolder)
	{
		m_tabs.Push(tab);
		tab->m_isPlacedHolder = true;

		m_tabHolder = tab;
	}
}

void EditorContext::SaveConfig()
{
	auto configFile = FileSystem::Get()->GetCurrentWorkingDirectory() + "Editor/Editor.config";

	json j = {};
	EditorSettings::Get()->WriteToJson(j);

	auto str = j.dump(2);
	FileUtils::WriteFile(configFile.c_str(), str.c_str(), str.length());
}

void EditorContext::LoadConfig()
{
	auto configFile = FileSystem::Get()->GetCurrentWorkingDirectory() + "Editor/Editor.config";
	if (!FileSystem::Get()->IsFileExisted(configFile))
	{
		return;
	}

	byte* str = nullptr; size_t size;
	FileUtils::ReadFile(configFile, str, size);

	json j = json::parse(str);
	EditorSettings::Get()->ReadFromJson(j);

	FileUtils::FreeBuffer(str);
}
