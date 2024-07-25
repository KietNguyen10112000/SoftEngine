#include "SceneEditorTabFactory.h"

#include "FileSystem/FileSystem.h"

#include "imgui/imgui.h"

#include "SceneEditorTab.h"
#include "DataInspector.h"
#include "SceneEditorSaveData.h"

SceneEditorTabFactory::SceneEditorTabFactory()
{
	m_tabKindName = "SceneEditorTabFactory";
}

void SceneEditorTabFactory::Begin()
{
	m_nameBuf[0] = 0;
	m_filePath = "";
	m_overwriteExist = false;
}

void SceneEditorTabFactory::End()
{
}

void SceneEditorTabFactory::ShowCreationInputGUI()
{
	ImGui::InputText("File name", m_nameBuf, IM_ARRAYSIZE(m_nameBuf));

	{
		Accessor temp = Accessor::ForString("Path", m_filePath, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_filePath;
		DataInspector::InspectStringPathEx(nullptr, temp, var, "File path", true);
	}
}

Handle<EditorTab> SceneEditorTabFactory::CreateInstance()
{
	auto ifdx = m_filePath.FindLastOf(".");
	auto ext = FileUtils::GetExtension(m_filePath);
	String tabName = m_nameBuf;

	if (ext == "json")
	{
		goto LoadJson;
	}

	if (m_nameBuf[0] == 0)
	{
		goto Failed;
	}

	if (tabName.empty() || (FileSystem::Get()->IsFileExist(SceneEditorTab::GetSavePath(tabName).c_str()) && !m_overwriteExist))
	{
		goto Failed;
	}
	else
	{
		// should I overwrite to the existed file
		EditorContext::Get()->OpenOkCancelDialog({},
			[](void* p)
			{
				auto self = (SceneEditorTabFactory*)p;
				String tabName = self->m_nameBuf;
				ImGui::TextUnformatted(String::Format("File \"{}\" existed!", SceneEditorTab::GetSavePath(tabName)).c_str());
			}, this,
			[](EditorContext::DIALOG_RESULT result, void* p) -> bool
				{
					auto self = (SceneEditorTabFactory*)p;

					if (result == EditorContext::DIALOG_RESULT::OK)
					{
						self->m_overwriteExist = true;
						EditorContext::Get()->CloseTabCreationPopUp();
					}

					return true;
				}, this
				);
	}

	goto Succeed;

Failed:
	return nullptr;

LoadJson:
	{
		auto tab = mheap::New<SceneEditorTab>();
		EditorContext::Get()->PlaceHolderTab(tab);

		Handle<Scene> scene;
		Serializer serializer = {};
		serializer.ReadFromFile(m_filePath);
		serializer.Deserialize(serializer.GetRootUUID(0), scene);

		Handle<SceneEditorSaveData> data;
		serializer.Deserialize(serializer.GetRootUUID(1), data);

		if (!scene)
		{
			std::cerr << "[SceneEditorTabFactory] - ERROR: Invalid file!\n";
			return nullptr;
		}

		auto fileName = FileUtils::GetLastName(m_filePath.c_str());
		tab->m_name = fileName.SubString(0, fileName.FindLastOf('.'));
		tab->m_scene = scene;

		tab->ReadSaveDataFromJson(&serializer, data->m_savedJson);

		EditorContext::Get()->PlaceHolderTab(nullptr);

		return tab;
	}

Succeed:
	auto scene = Runtime::Get()->CreateScene();
	auto tab = mheap::New<SceneEditorTab>();
	tab->m_scene = scene;
	tab->m_name = m_nameBuf;

	return tab;
}
