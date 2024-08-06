#pragma once
#include "SceneEditorTab.h"

class GameObjectEditorTab : public SceneEditorTab
{
	TRACEABLE_FRIEND();
public:
	GameObject* m_rootObject = nullptr;

	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;

	void OnShow() override;

	inline virtual String GetTabClassName() const override
	{
		return "GameObjectEditor";
	}

	using SceneEditorTab::OnObjectSelected;

};

