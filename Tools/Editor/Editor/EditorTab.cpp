#include "EditorTab.h"

#include "FileSystem/FileSystem.h"

void EditorTab::SetSaveFilePath(const String& dir)
{
	if (!FileSystem::Get()->IsFileExisted(dir.c_str()))
	{
		std::cerr << "[ERROR]: EditorTab::SetSaveFilePath() error!\n";
		return;
	}

	auto fileWithExtension = FileUtils::GetLastName(dir.c_str());
	auto fileName = fileWithExtension.SubString(0, fileWithExtension.RFind('.'));
	if (!EditorContext::Get()->IsVariableNameValid(fileName))
	{
		std::cerr << "[ERROR]: invalid file name!";
	}
	else
	{
		m_name = fileName;
		m_saveDirectory = FileUtils::PopPath(dir);
	}
}
