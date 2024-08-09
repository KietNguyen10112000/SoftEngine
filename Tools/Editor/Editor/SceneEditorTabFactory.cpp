#include "SceneEditorTabFactory.h"

#include "FileSystem/FileSystem.h"

#include "imgui/imgui.h"

#include "SceneEditorTab.h"
#include "DataInspector.h"
#include "SceneEditorSaveData.h"

SceneEditorTabFactory::SceneEditorTabFactory()
{
	m_tabKindName = "SceneEditorTab";
}

void SceneEditorTabFactory::Begin()
{
	m_nameBuf[0] = 0;
	m_filePath = "";
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

	if (ext == "json" || ext == "SceneEditor")
	{
		goto LoadJson;
	}

	if (m_nameBuf[0] == 0)
	{
		goto Failed;
	}

	/*if (AskIfExisted(SceneEditorTab::GetSavePath(tabName)))
	{
		goto Failed;
	}*/

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

		if (!scene || !data)
		{
			std::cerr << "[SceneEditorTabFactory] - ERROR: Invalid file!\n";
			EditorContext::Get()->PlaceHolderTab((EditorTab*)INVALID_ID);
			return nullptr;
		}

		auto fileName = FileUtils::GetLastName(m_filePath.c_str());
		tab->m_name = fileName.SubString(0, fileName.FindLastOf('.'));
		tab->m_scene = scene;

		tab->SetSaveFilePath(m_filePath);

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
