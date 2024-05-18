#include "AnimatorEditorTabFactory.h"

#include "AnimatorEditorTab.h"
#include "DataInspector.h"

#include "imgui/imgui.h"

#include "FileSystem/FileSystem.h"

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
	{
		Accessor temp = Accessor::ForString("Path", m_modelPath, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_modelPath;
		DataInspector::InspectStringPath(nullptr, temp, var, "Model path");
	}

	ImGui::InputText("File name", m_nameBuf, IM_ARRAYSIZE(m_nameBuf));
}

Handle<EditorTab> AnimatorEditorTabFactory::CreateInstance()
{
	if (m_modelPath.empty())
	{
		goto Failed;
	}

	if (m_nameBuf[0] == 0)
	{
		goto Failed;
	}

	goto Succeed;

Failed:
	return nullptr;

Succeed:
	m_name = m_nameBuf;

	auto scene = Runtime::Get()->CreateScene();
	auto tab = mheap::New<AnimatorEditorTab>(m_modelPath, scene);
	tab->m_scene = scene;
	tab->m_name = m_name;

	return tab;
}
