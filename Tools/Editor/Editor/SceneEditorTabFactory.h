#pragma once
#include "EditorTabFactory.h"

class SceneEditorTabFactory : public EditorTabFactory
{
public:
	char m_nameBuf[256] = {};

	String m_filePath;

	bool m_overwriteExist = false;

	SceneEditorTabFactory();

	// Inherited via EditorTabFactory
	void Begin() override;
	void End() override;
	void ShowCreationInputGUI() override;
	Handle<EditorTab> CreateInstance() override;

};

