#include "EditorTabFactory.h"

#include "SceneEditorTabFactory.h"
#include "AnimatorEditorTabFactory.h"
#include "GameObjectEditorTabFactory.h"

#include "FileSystem/FileSystem.h"

#include "imgui/imgui.h"

EditorTabFactoryManager::EditorTabFactoryManager()
{
	RegisterFactory<SceneEditorTabFactory>();
	RegisterFactory<AnimatorEditorTabFactory>();
	RegisterFactory<GameObjectEditorTabFactory>();
}

EditorTabFactoryManager::~EditorTabFactoryManager()
{
	for (auto& [key, value] : m_map)
	{
		delete value;
	}

	m_map.clear();
}

bool EditorTabFactory::AskIfExisted(const String& path)
{
	if (FileSystem::Get()->IsFileExisted(path.c_str()))
	{
		m_pathToCheckExist = path;
		if (!m_overwriteExist)
		{
			// should I overwrite to the existed file
			EditorContext::Get()->OpenOkCancelDialog({},
				[](void* p)
				{
					auto self = (EditorTabFactory*)p;
					ImGui::TextUnformatted(String::Format("File \"{}\" existed. Override it???", self->m_pathToCheckExist).c_str());
				}, this,
				[](EditorContext::DIALOG_RESULT result, void* p) -> bool
				{
					auto self = (EditorTabFactory*)p;

					if (result == EditorContext::DIALOG_RESULT::OK)
					{
						EditorContext::Get()->CloseTabCreationPopUp();
						self->m_overwriteExist = true;
						auto tab = self->CreateInstance();
						EditorContext::Get()->RunTab(tab);
					}
					else
					{
						EditorContext::Get()->OpenTabCreationPopUp();
					}

					return true;
				}, this
			);

			return true;
		}
	}

	return false;
}
