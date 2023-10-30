#include "EditorContext.h"

#include "imgui/imgui.h"

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

void EditorContext::RenderHierarchyPanel()
{
	ImGui::Begin("Hierarchy", 0, ImGuiWindowFlags_NoMove);

	ImGui::Text("Total %d objects in scene", m_objects.size());

	auto numObjects = m_objects.size();
	for (size_t i = 0; i < numObjects; i++)
	{
		auto _obj = m_objects[i];

		_obj->PreTraversal1(
			[&](GameObject* obj) 
			{
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanFullWidth;

				if (m_selectionIdx == i)
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
					m_selectionIdx = i;
					OnObjectSelected(obj);
				}

				if (open)
				{
					ImGui::TreePop();
				}
			}
		);
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
	auto& metaData = m_inspectingObjectData;

	ImGui::Begin("Inspector", 0, ImGuiWindowFlags_NoMove);

	if (m_inspectingObject.Get() == nullptr)
	{
		ImGui::Text("No object selected to inspect");
		goto Return;
	}

	metaData->ForEachProperties(
		[&](ClassMetadata*, const char* propertyName, Accessor& accessor)
		{
			ImGui::Text(propertyName);
			DisplayTransform(accessor, accessor.Get());
			ImGui::Separator();
		}
	);

Return:
	ImGui::End();
}

void EditorContext::DisplayTransform(Accessor& accessor, const Variant& variant)
{
	Transform transform = variant.As<Transform>();

	Vec3 euler = transform.Rotation().ToEulerAngles();

	bool modified = false;
	modified |= ImGui::DragFloat3("Scale", &transform.Scale()[0], 0.01f, -INFINITY, INFINITY);
	modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.01f, -INFINITY, INFINITY);
	modified |= ImGui::DragFloat3("Position", &transform.Position()[0], 0.01f, -INFINITY, INFINITY);

	if (modified)
	{
		transform.Rotation() = Quaternion(euler);

		auto input = Variant::Of<Transform>();
		input.As<Transform>() = transform;
		accessor.Set(input);
	}
}

void EditorContext::OnRenderGUI()
{
	RenderHierarchyPanel();
	RenderInspectorPanel();

	ImGui::ShowDemoWindow(0);
}
