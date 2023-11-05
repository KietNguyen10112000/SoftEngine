#include "EditorContext.h"

#include "imgui/imgui.h"
#include "DataInspector.h"

void EditorContext::OnObjectsAdded(std::vector<GameObject*>& objects)
{
	for (auto& obj : objects)
	{
		if (!obj->HasComponent<GameObjectEditorComponent>())
		{
			obj->NewComponent<GameObjectEditorComponent>();
		}

		auto editorComp = obj->GetComponentRaw<GameObjectEditorComponent>();
		editorComp->id = m_objects.size();
		m_objects.push_back(obj);
	}
}

void EditorContext::OnObjectsRemoved(std::vector<GameObject*>& objects)
{
	for (auto& obj : objects)
	{
		assert(obj->HasComponent<GameObjectEditorComponent>());

		auto editorComp = obj->GetComponentRaw<GameObjectEditorComponent>();
		auto blankIdx = editorComp->id;
		STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_objects, blankIdx);
		m_objects[blankIdx]->GetComponentRaw<GameObjectEditorComponent>()->id = blankIdx;
	}
}

void EditorContext::OnObjectSelected(GameObject* obj)
{
	m_inspectingObject = obj;
	m_inspectingObjectData = obj->GetMetadata(0);
}

void EditorContext::RenderHierarchyPanelOf(GameObject* _obj)
{
	auto preFunc = [&](GameObject* obj)
	{
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanFullWidth;

		if (m_selectionIdx == obj->UID())
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		auto open = ImGui::TreeNodeEx((void*)(intptr_t)obj->UID(),
			nodeFlags, obj->Name().empty() ? "<Unnamed>" : obj->Name().c_str());

		// right-click popup menu on object name
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Button("Rename"))
			{
				m_nameInputTxt[0] = 0;
				m_renameObject = obj;
				m_openInputNamePopup = true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_selectionIdx = obj->UID();
			OnObjectSelected(obj);
		}

		/*if (open)
		{
			ImGui::TreePop();
		}*/

		return open;
	};

	bool open = preFunc(_obj);

	if (open)
	{
		auto& children = _obj->Children();
		for (auto& child : children)
		{
			RenderHierarchyPanelOf(child.Get());
		}
	}

	if (open)
	{
		ImGui::TreePop();
	}
}

void EditorContext::RenderHierarchyPanel()
{
	ImGui::Begin("Hierarchy", 0, ImGuiWindowFlags_NoMove);

	ImGui::Text("Total %d objects in scene", m_objects.size());

	auto numObjects = m_objects.size();
	for (size_t i = 0; i < numObjects; i++)
	{
		auto _obj = m_objects[i];
		RenderHierarchyPanelOf(_obj);
	}

	if (m_openInputNamePopup)
	{
		ImGui::OpenPopup("#rename");
		m_openInputNamePopup = false;
	}

	if (ImGui::BeginPopup("#rename"))
	{
		if (m_renameObject && ImGui::InputText("Name", m_nameInputTxt, NAME_INPUT_MAX_LEN, 
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			m_renameObject->Name() = m_nameInputTxt;

			m_renameObject = nullptr;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
	

Return:
	ImGui::End();
}

void EditorContext::RenderInspectorPanel()
{
	size_t currentDepth = -1; 
	bool _open = false;

	auto& metaData = m_inspectingObjectData;

	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	if (m_pinInspectPanel)
	{
		wflags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	}

	ImGui::Begin("Inspector", 0, wflags);

	ImGui::Checkbox("Pin Inspector", &m_pinInspectPanel);
	
	if (m_inspectingObject.Get() == nullptr)
	{
		ImGui::Text("No object selected to inspect");
		goto Return;
	}

	if (!m_inspectingObject->Name().empty())
		ImGui::Text("Object: %s", m_inspectingObject->Name().c_str());

	ImGui::Separator();

	metaData->ForEachProperties(
		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			if (depth == 0)
			{
				ImGui::SetNextItemOpen(true);
			}

			if (currentDepth + 1 == depth)
			{
				auto open = ImGui::TreeNode(metadata->GetName());
				m_inspectPropertiesIsOpenStack.push_back(open);
				m_inspectInlinePropertiesCountStack.push_back(metadata->GetInlinePropertiesCount());
				currentDepth++;
			}

			bool open = m_inspectPropertiesIsOpenStack.back();
			if (open)
			{
				ImGui::Text(propertyName);
				DataInspector::Inspect(metadata, accessor, propertyName);
				ImGui::Separator();
				return true;
			}

			return false;
		},

		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			if (currentDepth == depth)
			{
				bool open = m_inspectPropertiesIsOpenStack.back();
				m_inspectPropertiesIsOpenStack.pop_back();
				m_inspectInlinePropertiesCountStack.pop_back();

				if (open)
					ImGui::TreePop();

				currentDepth--;
			}
		}
	);

	assert(m_inspectPropertiesIsOpenStack.empty());

Return:
	ImGui::End();
}

void EditorContext::OnRenderGUI()
{
	RenderHierarchyPanel();
	RenderInspectorPanel();

	ImGui::ShowDemoWindow(0);
}
