#pragma once
#include <vector>

#include "Scene/GameObject.h"

using namespace soft;

struct GameObjectEditorComponent
{
	ID id;
};

class EditorContext : Traceable<EditorContext>
{
public:
	constexpr static size_t NAME_INPUT_MAX_LEN = 2048;

	Scene* m_scene;

	std::vector<GameObject*> m_objects;

	Handle<GameObject> m_inspectingObject;
	Handle<ClassMetadata> m_inspectingObjectData;

	size_t m_selectionIdx = -1;

	bool m_openInputNamePopup = false;
	GameObject* m_renameObject = nullptr;
	char m_nameInputTxt[NAME_INPUT_MAX_LEN] = {};

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_inspectingObject);
		tracer->Trace(m_inspectingObjectData);
	}

	void RenderHierarchyPanel();
	void RenderInspectorPanel();

public:
	void OnObjectsAdded(std::vector<GameObject*>& objects);
	void OnObjectsRemoved(std::vector<GameObject*>& objects);
	void OnRenderGUI();

};

