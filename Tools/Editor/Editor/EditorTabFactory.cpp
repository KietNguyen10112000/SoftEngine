#include "EditorTabFactory.h"

#include "AnimatorEditorTabFactory.h"

EditorTabFactoryManager::EditorTabFactoryManager()
{
	RegisterFactory<AnimatorEditorTabFactory>();
}

EditorTabFactoryManager::~EditorTabFactoryManager()
{
	for (auto& [key, value] : m_map)
	{
		delete value;
	}

	m_map.clear();
}