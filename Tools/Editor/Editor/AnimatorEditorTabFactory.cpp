#include "AnimatorEditorTabFactory.h"

#include "AnimatorEditorTab.h"
#include "DataInspector.h"

#include "imgui/imgui.h"

#include "FileSystem/FileSystem.h"

#include "AnimatorEditorSaveData.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

AnimatorEditorTabFactory::AnimatorEditorTabFactory()
{
	m_tabKindName = "AnimatorEditorTab";
}

void AnimatorEditorTabFactory::Begin()
{
	m_nameBuf[0] = 0;
	m_modelPath = "";
}

void AnimatorEditorTabFactory::End()
{
}

void AnimatorEditorTabFactory::ShowCreationInputGUI()
{
	ImGui::InputText("File name", m_nameBuf, IM_ARRAYSIZE(m_nameBuf));

	{
		Accessor temp = Accessor::ForString("Path", m_modelPath, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_modelPath;
		DataInspector::InspectStringPathEx(nullptr, temp, var, "Model path", true);
	}
}

Handle<EditorTab> AnimatorEditorTabFactory::CreateInstance()
{
	auto ifdx = m_modelPath.FindLastOf(".");
	auto ext = FileUtils::GetExtension(m_modelPath);
	String tabName = m_nameBuf;

	if (ext == "json" || ext == "AnimatorEditor")
	{
		goto LoadJson;
	}

	if (m_modelPath.empty())
	{
		goto Failed;
	}

	if (m_nameBuf[0] == 0)
	{
		goto Failed;
	}

	/*if (AskIfExisted(AnimatorEditorTab::GetSavePath(tabName)))
	{
		goto Failed;
	}*/

	goto Succeed;

Failed:
	return nullptr;

LoadJson:
	{
		Handle<AnimatorEditorSaveData> data;
		Serializer serializer = {};
		serializer.ReadFromFile(m_modelPath);
		serializer.Deserialize(serializer.GetRootUUID(), data);

		if (!data)
		{
			std::cerr << "[AnimatorEditorTabFactory] - ERROR: Invalid file!\n";
			return nullptr;
		}

		auto tab = mheap::New<AnimatorEditorTab>(m_modelPath, nullptr, data->m_name);
		tab->m_name = data->m_name;
		tab->SetSaveFilePath(m_modelPath);

		Handle<GameObject> obj;
		serializer.Deserialize(data->m_objectUUID, obj);

		if (!obj)
		{
			std::cerr << "[AnimatorEditorTabFactory] - ERROR: Invalid file!\n";
			return nullptr;
		}

		Handle<GameObject> cam;
		serializer.Deserialize(data->m_cameraUUID, cam);

		if (!cam)
		{
			std::cerr << "[AnimatorEditorTabFactory] - ERROR: Invalid file!\n";
			return nullptr;
		}

		tab->m_object = obj;
		tab->m_objMetadata = obj->GetMetadata(0);
		tab->m_animator = obj->GetComponent<AnimatorSkeletalArray>();
		tab->m_cam = cam;

		Handle<Scene> scene;
		serializer.Deserialize(data->m_sceneUUID, scene);
		tab->m_scene = scene;

		tab->ReadNodeDataFromJson(&serializer, data->m_savedJson);

		return tab;
	}

Succeed:
	m_name = m_nameBuf;

	auto scene = Runtime::Get()->CreateScene();
	auto tab = mheap::New<AnimatorEditorTab>(m_modelPath, scene, m_name);
	tab->m_scene = scene;
	tab->m_name = m_name;

	return tab;
}
