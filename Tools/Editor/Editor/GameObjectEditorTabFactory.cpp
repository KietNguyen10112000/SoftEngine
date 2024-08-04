#include "GameObjectEditorTabFactory.h"

#include "FileSystem/FileSystem.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include "MainSystem/Physics/Materials/PhysicsMaterial.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Components/RigidBodyStatic.h"

#include "imgui/imgui.h"

#include "GameObjectEditorTab.h"
#include "DataInspector.h"
#include "GameObjectEditorSaveData.h"

GameObjectEditorTabFactory::GameObjectEditorTabFactory()
{
	m_tabKindName = "GameObjectEditorTabFactory";
}

void GameObjectEditorTabFactory::Begin()
{
	m_nameBuf[0] = 0;
	m_filePath = "";
}

void GameObjectEditorTabFactory::End()
{
}

void GameObjectEditorTabFactory::ShowCreationInputGUI()
{
	ImGui::InputText("File name", m_nameBuf, IM_ARRAYSIZE(m_nameBuf));

	{
		Accessor temp = Accessor::ForString("Path", m_filePath, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_filePath;
		DataInspector::InspectStringPathEx(nullptr, temp, var, "File path", true);
	}
}

Handle<EditorTab> GameObjectEditorTabFactory::CreateInstance()
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

	if (AskIfExisted(GameObjectEditorTab::GetSavePath(tabName)))
	{
		goto Failed;
	}

	goto Succeed;

Failed:
	return nullptr;

LoadJson:
	{
		auto tab = mheap::New<GameObjectEditorTab>();
		EditorContext::Get()->PlaceHolderTab(tab);

		Handle<Scene> scene;
		Serializer serializer = {};
		serializer.ReadFromFile(m_filePath);
		serializer.Deserialize(serializer.GetRootUUID(0), scene);

		Handle<GameObjectEditorSaveData> data;
		serializer.Deserialize(serializer.GetRootUUID(1), data);

		if (!scene || !data)
		{
			std::cerr << "[GameObjectEditorTabFactory] - ERROR: Invalid file!\n";
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
	auto tab = mheap::New<GameObjectEditorTab>();
	EditorContext::Get()->PlaceHolderTab(tab);
	tab->m_scene = scene;
	tab->m_name = m_nameBuf;

	// initialize some default objects

	auto cameraObj = mheap::New<GameObject>();
	cameraObj->Name() = "#camera";
	auto fppCamScript = cameraObj->NewComponent<FPPCameraScript>();
	auto camera = cameraObj->NewComponent<CameraTPP>();
	camera->Projection().SetPerspectiveFovLH(
		PI / 3.0f,
		Graphics::Get()->GetWindowWidth() / (float)Graphics::Get()->GetWindowHeight(),
		0.5f,
		1000.0f
	);
	camera->SetTPPEnabled(false);
	fppCamScript->SetFPPScriptEnable(true);
	scene->AddObject(cameraObj);

	auto material = std::make_shared<PhysicsMaterial>(0.9f, 0.9f, 0.6f);

	{
		auto obj = mheap::New<GameObject>();
		obj->Name() = "Ground";
		obj->NewComponent<MeshBasicRenderer>("Default/cube1.obj", "Default/white.png");

		auto shape = std::make_shared<PhysicsShapePlane>(material);
		obj->NewComponent<RigidBodyStatic>(shape);

		Transform transform = {};
		transform.Scale() = { 0.01f, 100.f, 100.f };
		transform.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, PI / 2);
		obj->SetLocalTransform(transform);

		scene->AddObject(obj);
	}

	{
		auto obj = mheap::New<GameObject>();
		obj->Name() = tab->m_name;
		tab->m_rootObject = obj;
		scene->AddObject(obj);

		tab->OnObjectSelected(obj);
	}

	EditorContext::Get()->PlaceHolderTab(nullptr);
	return tab;
}
