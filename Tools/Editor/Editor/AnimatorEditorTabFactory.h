#pragma once

#include "EditorTabFactory.h"

class AnimatorEditorTabFactory : public EditorTabFactory
{
public:
	char m_nameBuf[256] = {};

	String m_name;
	String m_modelPath;

	bool m_overwriteExist = false;

	AnimatorEditorTabFactory();

	// Inherited via EditorTabFactory
	void Begin() override;
	void End() override;
	void ShowCreationInputGUI() override;
	Handle<EditorTab> CreateInstance() override;
};

