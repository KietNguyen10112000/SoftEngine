#include "EditorContext.h"

#include "imgui/imgui.h"
#include "DataInspector.h"

#include "Scene/Scene.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"
#include "MainSystem/Scripting/Components/Script.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "ComponentInspector.h"

EditorContext::EditorContext()
{
	ReloadSerializableList();
}

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

		/*if (blankIdx == m_objects.size() - 1)
		{
			m_objects.pop_back();
		}
		else
		{
			STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_objects, blankIdx);
			m_objects[blankIdx]->GetComponentRaw<GameObjectEditorComponent>()->id = blankIdx;
		}*/

		m_objects.erase(m_objects.begin() + blankIdx);
		for (size_t i = blankIdx; i < m_objects.size(); i++)
		{
			m_objects[i]->GetComponentRaw<GameObjectEditorComponent>()->id--;
		}
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

		if (obj->Parent().Get() == nullptr && m_searchNameIdx == obj->GetComponentRaw<GameObjectEditorComponent>()->id)
		{
			ImGui::SetScrollHereY();
		}

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

			if (ImGui::Button("Delete"))
			{
				if (obj->Parent().Get() != nullptr)
				{
					obj->RemoveFromParent();
				}
				else
				{
					m_scene->RemoveObject(obj);
				}
			}

			ImGui::EndPopup();
		}

		ImGuiDragDropFlags src_flags = 0;
		src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
		//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip
		if (ImGui::BeginDragDropSource(src_flags))
		{
			m_dragingObject = obj;
			if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
				ImGui::Text("Moving");
			ImGui::SetDragDropPayload("TREE_DND_PAYLOAD", &obj, sizeof(obj));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TREE_DND_PAYLOAD", target_flags);
			if (payload && ImGui::IsMouseReleased(0))
			{
				auto dragObj = *(GameObject**)payload->Data;
				if (dragObj->Parent().Get() == nullptr)
				{
					dragObj->GetScene()->RemoveObject(dragObj);
				}
				else
				{
					dragObj->RemoveFromParent();
				}

				auto mat = dragObj->ReadGlobalTransformMat() * obj->ReadGlobalTransformMat().GetInverse();
				
				Transform transform;
				mat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

				dragObj->SetLocalTransform(transform);

				obj->AddChild(dragObj);

				m_dragingObject = nullptr;
			}

			ImGui::EndDragDropTarget();
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
	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	if (m_pinHierarchyPanel)
	{
		wflags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	}

	ImGui::Begin("Hierarchy", 0, wflags);

	ImGui::Checkbox("Pin Hierarchy Panel", &m_pinHierarchyPanel);
	if (ImGui::Button("New object"))
	{
		ImGui::OpenPopup("Create GameObject##CreateGameObjectPopup");
	}

	if (ImGui::InputText("Search object", m_searchNameInputTxt, NAME_INPUT_MAX_LEN,
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
	{
		/*if (m_searchNameIdx == INVALID_ID)
		{
			m_searchNameIdx = 0;
		}*/

		bool found = false;
		if (m_searchNameInputTxt[0] != 0)
		{
			for (size_t i = 1; i < m_objects.size(); i++)
			{
				auto idx = (m_searchNameIdx + i) % m_objects.size();
				auto& obj = m_objects[idx];
				if (!obj->Name().empty() && obj->Name() == m_searchNameInputTxt)
				{
					found = true;
					m_searchNameIdx = i;
				}
			}
		}

		if (!found)
		{
			m_searchNameIdx = -1;
		}
	}

	ImGui::Separator();

	ImGui::Text("Total %d objects in scene", m_objects.size());

	ImGui::BeginChild("Child");

	auto numObjects = m_objects.size();
	for (size_t i = 0; i < numObjects; i++)
	{
		auto _obj = m_objects[i];
		RenderHierarchyPanelOf(_obj);
	}

	ImGui::EndChild();

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

	//if (ImGui::BeginDragDropTarget())
	//{
	//	ImGuiDragDropFlags target_flags = 0;
	//	target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
	//	target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
	//	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TREE_DND_PAYLOAD", target_flags);
	//	std::cout << "Drop----\n";
	//	if (payload && ImGui::IsMouseReleased(0))
	//	{
	//		/*auto dragObj = *(GameObject**)payload->Data;
	//		if (dragObj->Parent().Get() == nullptr)
	//		{
	//			dragObj->GetScene()->RemoveObject(dragObj);
	//		}
	//		else
	//		{
	//			dragObj->RemoveFromParent();
	//		}

	//		m_scene->AddObject(dragObj);*/

	//		std::cout << "Drop\n";
	//	}

	//	ImGui::EndDragDropTarget();
	//}

	if (m_dragingObject && ImGui::IsMouseReleased(0))
	{
		auto dragObj = m_dragingObject;
		if (dragObj->Parent().Get() == nullptr)
		{
			dragObj->GetScene()->RemoveObject(dragObj);
		}
		else
		{
			dragObj->RemoveFromParent();
		}

		auto mat = dragObj->ReadGlobalTransformMat();

		Transform transform;
		mat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

		dragObj->SetLocalTransform(transform);

		m_scene->AddObject(dragObj);

		m_dragingObject = nullptr;
	}

	ShowCreateGameObjectPopup();

Return:
	ImGui::End();
}

void EditorContext::RenderInspectorPanel()
{
	if (m_needReloadInspectingObject)
	{
		if (m_inspectingObject)
		{
			m_inspectingObjectData = m_inspectingObject->GetMetadata(0);
		}
		m_needReloadInspectingObject = false;
	}

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
		ImGui::End();
		return;
	}

	if (!m_inspectingObject->Name().empty())
		ImGui::Text("Object: %s", m_inspectingObject->Name().c_str());

	ImGui::Separator();

	Inspect(metaData.Get());

	assert(m_inspectPropertiesIsOpenStack.empty());

	if (ImGui::Button("+##AddComponentBtn", ImVec2(ImGui::GetWindowWidth(), 0)))
	{
		ImGui::OpenPopup("Create component##AddComponentPopup");
	}

	ShowCreateComponentPopup();

	if (m_removeComp)
	{
		if (dynamic_cast<RenderingComponent*>(m_removeComp))
		{
			m_inspectingObject->RemoveComponentRaw(dynamic_cast<RenderingComponent*>(m_removeComp));
		}

		if (dynamic_cast<Script*>(m_removeComp))
		{
			m_inspectingObject->RemoveComponentRaw(dynamic_cast<Script*>(m_removeComp));
		}

		/*if (dynamic_cast<RenderingComponent*>(m_removeComp))
		{
			m_inspectingObject->RemoveComponentRaw(dynamic_cast<RenderingComponent*>(m_removeComp));
		}*/

		m_removeComp = nullptr;

		m_needReloadInspectingObject = true;
	}

//Return:
	ImGui::End();
}

void EditorContext::RenderMenuBar()
{
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File")) 
	{
		if (ImGui::MenuItem("Reload")) 
		{
			ReloadSerializableList();
		}

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EditorContext::ShowCreateComponentPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y / 2));
	if (ImGui::BeginPopupModal("Create component##AddComponentPopup", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImGui::Separator();

		ImGui::BeginChild("Content", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 100));

		const char* previewComponentType = nullptr;
		if (m_createComponentContext.selectedComponentId != INVALID_ID)
		{
			previewComponentType = MainSystemInfo::COMPONENT_NAME[m_createComponentContext.selectedComponentId];
		}

		if (ImGui::BeginCombo("Component Type", previewComponentType))
		{
			for (size_t n = 0; n < MainSystemInfo::COUNT; n++)
			{
				if (MainSystemInfo::COMPONENT_NAME[n] && !m_inspectingObject->HasComponent(n))
				{
					if (ImGui::Selectable(MainSystemInfo::COMPONENT_NAME[n], n == m_createComponentContext.selectedComponentId))
					{
						m_createComponentContext.selectedComponentId = n;
						m_createComponentContext.selectedIdx = INVALID_ID;
					}
				}
			}

			ImGui::EndCombo();
		}

		const char* previewComponentClass = nullptr;
		if (m_createComponentContext.selectedComponentId != INVALID_ID && m_createComponentContext.selectedIdx != INVALID_ID)
		{
			previewComponentClass = m_components[m_createComponentContext.selectedComponentId][m_createComponentContext.selectedIdx]->name.c_str();
		}

		if (ImGui::BeginCombo("Component Class", previewComponentClass))
		{
			if (m_createComponentContext.selectedComponentId != INVALID_ID)
			{
				auto& components = m_components[m_createComponentContext.selectedComponentId];
				for (size_t n = 0; n < components.size(); n++)
				{
					auto componentRecord = components[n];
					if (ImGui::Selectable(componentRecord->name.c_str(), n == m_createComponentContext.selectedIdx))
					{
						m_createComponentContext.selectedIdx = n;
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 100, ImGui::GetWindowHeight() - 40));

		if (ImGui::Button("OK", ImVec2(100, 0)))
		{
			if (m_createComponentContext.selectedComponentId != INVALID_ID && m_createComponentContext.selectedIdx != INVALID_ID)
			{
				auto compCtor = m_components[m_createComponentContext.selectedComponentId][m_createComponentContext.selectedIdx]->ctor;
				auto comp = compCtor();
				switch (m_createComponentContext.selectedComponentId)
				{
				case MainSystemInfo::RENDERING_ID:
					m_inspectingObject->AddComponent(DynamicCast<RenderingComponent>(comp));
					break;
				case MainSystemInfo::PHYSICS_ID:
					break;
				case MainSystemInfo::SCRIPTING_ID:
					m_inspectingObject->AddComponent(DynamicCast<Script>(comp));
					break;
				default:
					assert(0);
					break;
				}
			}

			m_needReloadInspectingObject = true;

			m_createComponentContext.selectedIdx = INVALID_ID;
			m_createComponentContext.selectedComponentId = INVALID_ID;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine(0, 20);
		if (ImGui::Button("Cancel", ImVec2(100, 0)))
		{
			m_createComponentContext.selectedIdx = INVALID_ID;
			m_createComponentContext.selectedComponentId = INVALID_ID;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void EditorContext::ShowCreateGameObjectPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y / 2));
	if (ImGui::BeginPopupModal("Create GameObject##CreateGameObjectPopup", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImGui::Separator();

		ImGui::BeginChild("Content", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 100));

		ImGui::InputText("Name", m_nameInputTxt, NAME_INPUT_MAX_LEN);

		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 100, ImGui::GetWindowHeight() - 40));

		if (ImGui::Button("OK", ImVec2(100, 0)))
		{
			auto gameObject = mheap::New<GameObject>();
			gameObject->Name() = m_nameInputTxt;
			m_scene->AddObject(gameObject);

			m_nameInputTxt[0] = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine(0, 20);
		if (ImGui::Button("Cancel", ImVec2(100, 0)))
		{
			m_nameInputTxt[0] = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void EditorContext::ReloadSerializableList()
{
	for (auto& v : m_components)
	{
		v.clear();
	}

	SerializableDB::Get()->ForEachSerializableRecord(
		[&](const SerializableDB::SerializableRecord& record)
		{
			if (record.COMPONENT_ID == INVALID_ID)
			{
				return;
			}

			m_components[record.COMPONENT_ID].push_back((SerializableDB::SerializableRecord*)&record);
		}
	);
}

void EditorContext::OnRenderGUI()
{
	RenderHierarchyPanel();
	RenderInspectorPanel();
	RenderMenuBar();

	ImGui::ShowDemoWindow(0);
}

void EditorContext::OnRenderInGameDebugGraphics()
{
//#ifdef _DEBUG
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics) return;

	if (m_inspectingObject)
	{
		auto mat = m_inspectingObject->GetLocalTransform().ToTransformMatrix();

		debugGraphics->DrawDirection(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
		debugGraphics->DrawDirection(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
		debugGraphics->DrawDirection(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });
	}

	debugGraphics->DrawDirection(Vec3(-10, 0, 0), Vec3(20, 0, 0), { 1,0,0,1 }, { 1,0,0,1 });
	debugGraphics->DrawDirection(Vec3(0, -10, 0), Vec3(0, 20, 0), { 0,1,0,1 }, { 0,1,0,1 });
	debugGraphics->DrawDirection(Vec3(0, 0, -10), Vec3(0, 0, 20), { 0,0,1,1 }, { 0,0,1,1 });


	// test
	debugGraphics->DrawSphere({ Vec3(0, 0, 0), 1 }, Vec4(1, 0, 0, 1));
	debugGraphics->DrawDirection(Vec3(0, 0, 0.5f), Vec3::X_AXIS, { 0,0,1,1 }, { 0,1,0,1 });

//#endif // _DEBUG
}

void EditorContext::Inspect(ClassMetadata* metaData)
{
	size_t currentDepth = -1;
	metaData->ForEachProperties(
		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			if (depth == 0)
			{
				ImGui::SetNextItemOpen(true);
			}

			bool rawInspect = true;
			if (currentDepth + 1 == depth)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.f));
				auto open = ImGui::TreeNodeEx(metadata->GetName(), ImGuiTreeNodeFlags_FramePadding);
				ImGui::PopStyleVar();

				m_inspectPropertiesIsOpenStack.push_back(open);
				m_inspectInlinePropertiesCountStack.push_back(metadata->GetInlinePropertiesCount());
				currentDepth++;

				if (depth != 0 && dynamic_cast<MainComponent*>(metadata->GetInstance()))
				{
					//ImGui::SameLine();
					auto pos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 50, pos.y - 35));
					String btnName = String("x") + "##" + metadata->GetName();
					if (ImGui::Button(btnName.c_str(), ImVec2(30, 30)))
					{
						m_removeComp = dynamic_cast<MainComponent*>(metadata->GetInstance());
					}

					ImGui::SetCursorPos(pos);
				}

				if (open)
				{
					if (ComponentInspector::Get()->Inspect(this, metadata->GetInstance(), metadata, propertyName))
					{
						rawInspect = false;
					}
				}

				m_inspectPropertiesIsRawInspectStack.push_back(rawInspect);
			}

			bool open = m_inspectPropertiesIsOpenStack.back();
			if (open && m_inspectPropertiesIsRawInspectStack.back())
			{
				ImGui::Text(propertyName);

				//if (!ComponentInspector::Get()->Inspect(this, metadata->GetInstance(), metadata))
				{
					DataInspector::Inspect(metadata, accessor, propertyName);
				}

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
				m_inspectPropertiesIsRawInspectStack.pop_back();

				if (open)
				{
					ImGui::TreePop();
				}

				currentDepth--;
			}
		}
	);
}
