#pragma once
#include "SceneEditorTab.h"

class GameObjectEditorTab : public SceneEditorTab
{
	TRACEABLE_FRIEND();
	using Base = SceneEditorTab;
public:
	GameObject* m_rootObject = nullptr;

	char m_exportInputName[256] = {};
	String m_exportResourcePath = "./";

	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;

	void OnShow() override;

	void WriteSaveDataToJson(Serializer* serializer, json& j) override;
	void ReadSaveDataFromJson(Serializer* serializer, const json& j) override;
	void OnRenderGameObjectContextMenu(GameObject* obj) override;

	bool ValidateSetting();
	void Export();

	inline virtual String GetTabClassName() const override
	{
		return "GameObjectEditor";
	}

	using SceneEditorTab::OnObjectSelected;

};

