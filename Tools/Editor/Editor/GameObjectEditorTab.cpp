#include "GameObjectEditorTab.h"

#include "GameObjectEditorSaveData.h"

#include "imgui/imgui.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "SystemDialog.h"
#include "DataInspector.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Joints/FixedJoint.h"

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

	if (ImGui::Button(ICON_FA_CIRCLE_PLUS " Object"))
	{
		auto obj = mheap::New<GameObject>();
		m_rootObject->AddChild(obj);
		IndexObject(obj);
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FILE_IMPORT " Import Model"))
	{
		SystemDialog::FileChooserDialog otp;
		otp.forceInsideResourcesPath = true;
		otp.extensionGroups = {
			{
				"3D Static Model File (*.obj, *.fbx, *.dae, *.stl)",
				{ "obj", "fbx", "dae", "stl" }
			}
		};

		if (SystemDialog::OpenFileChooser(otp))
		{
			auto obj = LoadStaticModelFromFile(otp.outputFilePath);
			if (obj)
			{
				m_rootObject->AddChild(obj);
			}
		}
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_GEAR " Setting"))
	{
		EditorContext::Get()->OpenOkCancelDialog({},
			[](void* p)
			{
				auto self = (GameObjectEditorTab*)p;
				if (ImGui::BeginTable("Exports", 2, ImGuiTableFlags_SizingFixedFit))
				{
					auto secondColumnWidth = 0.7f * ImGui::GetWindowWidth();
					{
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Export Name"); ImGui::SameLine();

						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(secondColumnWidth);
						ImGui::InputText("## Edit export name", self->m_exportInputName, sizeof(self->m_exportInputName));
					}

					{
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Export Resource Path"); ImGui::SameLine();

						ImGui::TableNextColumn();
						Accessor temp = Accessor::ForString("Path", self->m_exportResourcePath, nullptr);
						Variant var = Variant(VARIANT_TYPE::STRING_PATH);
						var.AsString() = self->m_exportResourcePath;
						DataInspector::InspectStringPathEx(nullptr, temp, var, "Export Resource Path", true, secondColumnWidth, true);
					}

					ImGui::EndTable();
				}
			}, this,
			[](EditorContext::DIALOG_RESULT result, void* p) -> bool
			{
				auto self = (GameObjectEditorTab*)p;
				if (!self->ValidateSetting())
				{
					return false;
				}

				return true;
			}, this
		);
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FILE_EXPORT " Export"))
	{
		Export();
	}

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
			auto path = self->GetSaveFilePath();

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

void GameObjectEditorTab::WriteSaveDataToJson(Serializer* serializer, json& j)
{
	Base::WriteSaveDataToJson(serializer, j);
	j["RootObject"] = serializer->Serialize(m_rootObject);

	j["ExportInputName"] = String(m_exportInputName);
	j["ExportResourcePath"] = m_exportResourcePath;
}

void GameObjectEditorTab::ReadSaveDataFromJson(Serializer* serializer, const json& j)
{
	Base::ReadSaveDataFromJson(serializer, j);
	serializer->Deserialize(j["RootObject"], m_rootObject);

	if (j.contains("ExportInputName"))
	{
		String exportInputName = j["ExportInputName"];
		if (!exportInputName.empty())
		{
			std::memcpy(m_exportInputName, exportInputName.c_str(), exportInputName.length() + 1);
		}

		m_exportResourcePath = j["ExportResourcePath"];
	}
}

void GameObjectEditorTab::OnRenderGameObjectContextMenu(GameObject* obj)
{
	/*if (ImGui::MenuItem("Test"))
	{
		auto body = m_rootObject->Children()[0]->GetComponentRaw<RigidBodyDynamic>();
		body->AddForce({ 0,10000,0 });

		auto& joint = body->GetJoint(0);
		joint->Break();
		joint->GetAnotherBody(body)->SetFamilyNoCollideForAllShapes(false);
	}*/
}

bool GameObjectEditorTab::ValidateSetting()
{
	bool ret = EditorContext::Get()->IsVariableNameValid(m_exportInputName);
	if (!ret)
	{
		std::cerr << "[ERROR]: Export Name illegal!\n";
	}

	return ret;
}

void GameObjectEditorTab::Export()
{
	if (!ValidateSetting())
	{
		return;
	}

	auto exportPath = m_exportResourcePath + m_exportInputName + ".json";
	Serializer s = {};
	s.Serialize(m_rootObject);
	s.SetRootUUID(m_rootObject->GetUUID());
	s.WriteToFile(exportPath);
}
