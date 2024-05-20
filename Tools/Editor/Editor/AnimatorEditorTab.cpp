#include "AnimatorEditorTab.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"

#include "DataInspector.h"
#include "AnimatorEditorSaveData.h"

#include "imgui/imgui.h"

#include "imgui-node-editor/imgui_node_editor.h"

namespace ed = ax::NodeEditor;

AnimatorEditorTab::AnimatorEditorTab(const String& modelPath, Scene* scene)
{
	m_modelPath = modelPath;
}

void AnimatorEditorTab::OnObjectsAdded(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnObjectsRemoved(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnRenderGUI()
{
	const float HEADER_HEIGHT = 65;

	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;

	{
		wflags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowPos({ center.x,HEADER_HEIGHT }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y - HEADER_HEIGHT));

		if (ImGui::Begin("Editor", 0, wflags))
		{
			RenderBluePrintPanel();
			ImGui::End();
		}
	}
	
	{
		wflags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowPos({ 0,HEADER_HEIGHT }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 4.0f, viewPortSize.y / 4.0f));
		ImGui::Begin("Transform", 0, wflags);

		m_objMetadata->ForEachProperties(
			[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
			{
				auto var = accessor.Get();
				if (var.Type() == VARIANT_TYPE::TRANSFORM3D)
				{
					DataInspector::Inspect(metadata, accessor, propertyName);
					return false;
				}

				return true;
			}, nullptr
		);

		
		ImGui::End();
	}
}

void AnimatorEditorTab::OnRenderInGameDebugGraphics()
{
	EditorContext::OxyzRenderConfig config;
	config.RenderOxzGrid = true;
	config.AxisYLength = 200.0f;
	EditorContext::GetInstance()->RenderOxyz(config);
}

void AnimatorEditorTab::OnShow()
{
	m_onSaveListenerId = EditorContext::Get()->EventDispatcher()->AddListener(EditorContext::EVENT::MENU_ON_SAVE,
		[](EditorContext* ctx, int argc, void** argv, ID id)
		{
			auto path = *(String*)argv[0];
			auto tab = (AnimatorEditorTab*)ctx->GetTab(id);

			AnimatorEditorSaveData data;
			data.m_name = tab->m_name;
			data.m_objectUUID = tab->m_object->GetUUID();
			data.m_sceneUUID = tab->m_scene->GetUUID();
			data.m_cameraUUID = tab->m_cam->GetUUID();

			Serializer serializer = {};
			serializer.Serialize(tab->m_scene);
			serializer.Serialize(&data);
			serializer.SetRootUUID(data.GetUUID());

			if (path.empty())
			{
				path = tab->m_modelPath;
			}

			auto ext = FileUtils::GetExtension(path);
			if (ext != "json")
			{
				path = EditorContext::Get()->GetSavePath() + "AnimatorEditor/" + tab->m_name + ".json";
			}

			serializer.WriteToFile(path);
		}, 
		m_id
	);
}

void AnimatorEditorTab::OnHide()
{
	if (m_onSaveListenerId != INVALID_ID)
	{
		EditorContext::Get()->EventDispatcher()->RemoveListener(m_onSaveListenerId);
		m_onSaveListenerId = INVALID_ID;
	}
}

void AnimatorEditorTab::OnOpen()
{
	auto savePath = (EditorContext::GetInstance()->GetSavePath() + "AnimatorEditor/" + m_name + ".config.json");
	ed::Config config;
    config.SettingsFile = savePath.c_str();
    config.UserPointer = this;
	m_nodeEditorCtx = ed::CreateEditor(&config);

	Transform transform = {};

	if (!m_cam)
	{
		auto cameraObj = mheap::New<GameObject>();
		cameraObj->Name() = "#camera";
		auto fppCamScript = cameraObj->NewComponent<FPPCameraScript>();
		auto camera = cameraObj->NewComponent<Camera>();
		camera->Projection().SetPerspectiveFovLH(
			PI / 3.0f,
			Graphics::Get()->GetWindowWidth() / 2.0f / (float)Graphics::Get()->GetWindowHeight(),
			0.5f,
			1000.0f
		);
		fppCamScript->SetFPPScriptEnable(true);
		m_scene->AddObject(cameraObj);

		m_cam = cameraObj;
	}

	auto cam = m_cam->GetComponent<Camera>();
	m_cam->GetComponent<FPPCameraScript>()->SetFPPScriptEnable(true);
	m_scene->GetRenderingSystem()->HideCamera(cam);
	m_scene->GetRenderingSystem()->DisplayCamera(cam,
		GRAPHICS_VIEWPORT({ {0,0},{Graphics::Get()->GetWindowWidth() / 2,Graphics::Get()->GetWindowHeight()} })
	);

	if (m_object)
	{
		return;
	}

	m_object = resource::Load<AnimModel>(m_modelPath)->MakeGameObject();
	m_animator = m_object->GetComponent<AnimatorSkeletalArray>();

	m_scene->AddObject(m_object);

	m_objMetadata = m_object->GetMetadata(0);
}

void AnimatorEditorTab::OnClose()
{
	ed::DestroyEditor(m_nodeEditorCtx);
	m_nodeEditorCtx = nullptr;
}

void AnimatorEditorTab::RenderBluePrintPanel()
{
	ed::SetCurrentEditor(m_nodeEditorCtx);
	ed::Begin("Node Editor", ImVec2(0.0, 0.0f));

	int uniqueId = 1;
    // Start drawing nodes.
    ed::BeginNode(uniqueId++);
        ImGui::Text("Node A");
        ed::BeginPin(uniqueId++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(uniqueId++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();

	ed::BeginNode(uniqueId++);
        ImGui::Text("Node B");
        ed::BeginPin(uniqueId++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(uniqueId++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();

	ed::End();
	ed::SetCurrentEditor(nullptr);
}
