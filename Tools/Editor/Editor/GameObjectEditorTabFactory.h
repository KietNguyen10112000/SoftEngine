#pragma once
#include "EditorTabFactory.h"

class GameObjectEditorTabFactory : public EditorTabFactory
{
public:
	char m_nameBuf[256] = {};

	String m_filePath;

	GameObjectEditorTabFactory();

	// Inherited via EditorTabFactory
	void Begin() override;
	void End() override;
	void ShowCreationInputGUI() override;
	Handle<EditorTab> CreateInstance() override;

};

