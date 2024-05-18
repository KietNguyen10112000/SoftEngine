#include "AnimatorEditorTab.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"

#include "imgui/imgui.h"

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
	ImGui::Begin("Scene2");

	ImGui::Text("Hello");

	ImGui::End();
}

void AnimatorEditorTab::OnRenderInGameDebugGraphics()
{
}

void AnimatorEditorTab::OnShow()
{
}

void AnimatorEditorTab::OnHide()
{
}

void AnimatorEditorTab::OnOpen()
{
	Transform transform = {};

	{
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
		m_scene->AddObject(cameraObj);
	}

	m_object = resource::Load<AnimModel>(m_modelPath)->MakeGameObject();
	m_animator = m_object->GetComponent<AnimatorSkeletalArray>();

	m_scene->AddObject(m_object);
}

void AnimatorEditorTab::OnClose()
{
}
