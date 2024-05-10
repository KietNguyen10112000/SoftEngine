#pragma once
#include "EditorTab.h"

class AnimatorEditorTab : public EditorTab
{
	// Inherited via EditorTab
	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;
};

