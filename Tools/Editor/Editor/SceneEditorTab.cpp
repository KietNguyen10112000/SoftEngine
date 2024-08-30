#include "SceneEditorTab.h"

#include "imgui/imgui.h"
#include "DataInspector.h"

#include "Scene/Scene.h"

#include "MainSystem/Rendering/Components/RenderingComponent.h"
#include "MainSystem/Scripting/Components/Script.h"
#include "MainSystem/Physics/Components/PhysicsComponent.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Joints/Joint.h"
#include "MainSystem/Animation/Components/AnimationComponent.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "ComponentInspector.h"
#include "SceneEditorSaveData.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"
#include "EditorFont.h"

#include "SystemDialog.h"

SceneEditorTab::SceneEditorTab()
{
}

void SceneEditorTab::OnObjectsAdded(std::vector<GameObject*>& objects)
{
	if (m_isHotDeserializingGameObjectFromFile)
	{
		return;
	}

	for (auto& obj : objects)
	{
		AddObjectToEditor(obj);
	}
}

void SceneEditorTab::OnObjectsRemoved(std::vector<GameObject*>& objects)
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

void SceneEditorTab::OnObjectSelected(GameObject* obj)
{
	if (m_inspectingObject == obj)
	{
		return;
	}

	if (m_inspectingObject)
	{
		ComponentInspector::EndInspectingFor(m_inspectingObject, m_inspectingObjectData, nullptr);
		m_inspectingObject = nullptr;
		m_inspectingObjectData = nullptr;
	}

	if (obj)
	{
		m_inspectingObject = obj;
	}

	if (m_inspectingObject)
	{
		m_inspectingObjectData = obj->GetMetadata(0);
	}
}

void SceneEditorTab::RenderHierarchyPanelOf(GameObject* _obj)
{
	auto preFunc = [&](GameObject* obj)
		{
			ImGuiTreeNodeFlags nodeFlags = 
				ImGuiTreeNodeFlags_OpenOnArrow 
				| ImGuiTreeNodeFlags_OpenOnDoubleClick 
				| ImGuiTreeNodeFlags_SpanAvailWidth 
				| ImGuiTreeNodeFlags_AllowItemOverlap 
				| ImGuiTreeNodeFlags_SpanFullWidth
				| ImGuiTreeNodeFlags_FramePadding;

			//if (m_selectionId == (ID)obj->GetComponentRaw<GameObjectEditorComponent>())
			if (obj == m_inspectingObject)
			{
				nodeFlags |= ImGuiTreeNodeFlags_Selected;
			}

			// place a small space to let one drag to
			bool selected = false;
			ImGui::Selectable(String::Format("## {}", obj).c_str(), &selected, ImGuiSelectableFlags_Disabled, {0,5});
			{
				if (ImGui::BeginDragDropTarget())
				{
					ImGuiDragDropFlags target_flags = 0;
					target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
					//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DND_PAYLOAD", target_flags);
					if (payload && ImGui::IsMouseReleased(0) &&
						(obj->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false))
					{
						auto dragObj = *(GameObject**)payload->Data;

						if (dragObj != obj)
						{
						RetryAddDragObject:
							if (dragObj->Parent().Get() == obj->Parent().Get())
							{
								if (dragObj->Parent().Get() == nullptr)
								{
									// move dragObj upper

									auto upperObjIdx = dragObj->GetComponentRaw<GameObjectEditorComponent>()->id;
									m_objects.erase(m_objects.begin() + upperObjIdx);
									ReindexObjects();

									auto lowerObjIdx = obj->GetComponentRaw<GameObjectEditorComponent>()->id;
									m_objects.insert(m_objects.begin() + lowerObjIdx, dragObj);
									ReindexObjects();
								}
								else
								{
									auto& children = *(Array<Handle<GameObject>>*)&dragObj->Parent()->Children();
									dragObj->Lock()->lock();
									obj->Lock()->lock();

									auto upperObjIdx = dragObj->ParentIdx();
									children.Remove(children.begin() + upperObjIdx);
									ReindexChildren(children);

									auto lowerObjIdx = obj->ParentIdx();
									children.insert(children.begin() + lowerObjIdx, dragObj);
									ReindexChildren(children);

									obj->Lock()->unlock();
									dragObj->Lock()->unlock();
								}
							}
							else
							{
								if (dragObj->Parent().Get() == nullptr)
								{
									dragObj->GetScene()->RemoveObject(dragObj);
								}
								else
								{
									dragObj->RemoveFromParent(true);
								}

								if (obj->Parent().Get() == nullptr)
								{
									m_scene->AddObject(dragObj);
								}
								else
								{
									obj->Parent()->AddChild(dragObj);
								}

								goto RetryAddDragObject;
							}
						}

						m_dragingObject = nullptr;
					}

					ImGui::EndDragDropTarget();
				}
			}

			if (obj->GetComponentRaw<GameObjectEditorComponent>()->expandAll)
			{
				ImGui::SetNextItemOpen(true);
				obj->GetComponentRaw<GameObjectEditorComponent>()->expandAll = false;
			}
			
			ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_FramePadding, { 0,2 });
			auto open = ImGui::TreeNodeEx((void*)(intptr_t)obj,
				nodeFlags, obj->Name().empty() ? "<Unnamed>" : obj->Name().c_str());
			ImGui::PopStyleVar();

			if (obj->Parent().Get() == nullptr && m_searchNameIdx == obj->GetComponentRaw<GameObjectEditorComponent>()->id)
			{
				ImGui::SetScrollHereY();
			}

			auto rectMin = ImGui::GetItemRectMin();
			auto rectMax = ImGui::GetItemRectMax();
			auto drawList = ImGui::GetWindowDrawList();

			if (m_highlightingObjects.find(obj) != m_highlightingObjects.end())
			{
				drawList->AddRect(rectMin, rectMax, IM_COL32(255, 255, 0, 255));
			}

			// right-click popup menu on object name
			if (ImGui::BeginPopupContextItem())
			{
				drawList->AddRect(rectMin, rectMax, IM_COL32(0, 255, 255, 255));
				RenderObjectContextPopup(obj);
				ImGui::EndPopup();
			}

			ImGuiDragDropFlags src_flags = 0;
			src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
			src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
			//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip
			src_flags |= ImGuiDragDropFlags_SourceAllowNullID;
			if (ImGui::BeginDragDropSource(src_flags))
			{
				// don't allow drag and drop on hot reloaded object children
				auto allowDragDrop = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;
				if (allowDragDrop)
				{
					m_dragingObject = obj;
					if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
						ImGui::Text("Moving");
					ImGui::SetDragDropPayload("GAMEOBJECT_DND_PAYLOAD", &obj, sizeof(obj));
				}
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				ImGuiDragDropFlags target_flags = 0;
				target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
				//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DND_PAYLOAD", target_flags);
				if (payload && ImGui::IsMouseReleased(0) && 
					(obj->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false))
				{
					auto dragObj = *(GameObject**)payload->Data;
					if (dragObj->Parent().Get() == nullptr)
					{
						dragObj->GetScene()->RemoveObject(dragObj);
					}
					else
					{
						dragObj->RemoveFromParent(true);
					}

					auto mat = dragObj->GetCommittedGlobalTransform() * obj->GetCommittedGlobalTransform().GetInverse();

					Transform transform;
					mat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

					dragObj->SetLocalTransform(transform);

					obj->AddChild(dragObj);

					m_dragingObject = nullptr;
				}

				ImGui::EndDragDropTarget();
			}

			if (m_dragingObject == nullptr && ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) && m_willbeSelectedObject == obj)
			{
				//std::cout << "Released\n";
				OnObjectSelected(obj);
			}

			if (m_dragingObject && ImGui::IsMouseReleased(0) && ImGui::IsItemHovered() && m_dragingObject == obj)
			{
				m_dragingObject = nullptr;
			}

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			{
				//m_selectionId = (ID)obj->GetComponentRaw<GameObjectEditorComponent>();
				//OnObjectSelected(obj);
				//std::cout << "Clicked\n";
				m_willbeSelectedObject = obj;
			}

			/*if (open)
			{
				ImGui::TreePop();
			}*/

			if (obj->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile)
			{
				bool enable = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;

				ImGui::SameLine(ImGui::GetWindowWidth() - 35, -1.0f);
				ImGui::BeginDisabled(!enable);
				ImGui::PushFont(EditorFont::Get()->GetFont(22));

				if (ImGui::Button(ICON_FA_FIRE))
				{

				}

				ImGui::PopFont();
				ImGui::EndDisabled();
				ImGui::Dummy({ 0, 0 });
			}

			// place a small space to let one drag to
			if (((obj->Parent().Get() == nullptr && obj == m_objects.back())
				|| (obj->Parent().Get() != nullptr && obj->Parent()->Children().back().Get() == obj))
				&& !(open && obj->Children().size() != 0))
			{
				bool selected = false;
				ImGui::Selectable(String::Format("## {}", obj).c_str(), &selected, ImGuiSelectableFlags_Disabled, { 0,5 });
				{
					if (ImGui::BeginDragDropTarget())
					{
						ImGuiDragDropFlags target_flags = 0;
						target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
						//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
						const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DND_PAYLOAD", target_flags);
						if (payload && ImGui::IsMouseReleased(0) &&
							(obj->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false))
						{
							auto dragObj = *(GameObject**)payload->Data;

							if (dragObj != obj)
							{
							RetryAddDragObject2:
								if (dragObj->Parent().Get() == obj->Parent().Get())
								{
									if (dragObj->Parent().Get() == nullptr)
									{
										// move dragObj lower

										auto upperObjIdx = dragObj->GetComponentRaw<GameObjectEditorComponent>()->id;
										m_objects.erase(m_objects.begin() + upperObjIdx);
										ReindexObjects();

										auto lowerObjIdx = obj->GetComponentRaw<GameObjectEditorComponent>()->id;
										m_objects.insert(m_objects.begin() + lowerObjIdx + 1, dragObj);
										ReindexObjects();
									}
									else
									{
										auto& children = *(Array<Handle<GameObject>>*) & dragObj->Parent()->Children();
										dragObj->Lock()->lock();
										obj->Lock()->lock();

										auto upperObjIdx = dragObj->ParentIdx();
										children.Remove(children.begin() + upperObjIdx);
										ReindexChildren(children);

										auto lowerObjIdx = obj->ParentIdx();
										children.insert(children.begin() + lowerObjIdx + 1, dragObj);
										ReindexChildren(children);

										obj->Lock()->unlock();
										dragObj->Lock()->unlock();
									}
								}
								else
								{
									if (dragObj->Parent().Get() == nullptr)
									{
										dragObj->GetScene()->RemoveObject(dragObj);
									}
									else
									{
										dragObj->RemoveFromParent(true);
									}

									if (obj->Parent().Get() == nullptr)
									{
										m_scene->AddObject(dragObj);
									}
									else
									{
										obj->Parent()->AddChild(dragObj);
									}

									goto RetryAddDragObject2;
								}
							}

							m_dragingObject = nullptr;
						}

						ImGui::EndDragDropTarget();
					}
				}
			}

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

void SceneEditorTab::RenderHierarchyPanel()
{
	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	if (m_pinHierarchyPanel)
	{
		wflags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	}

	ImGui::Begin("Hierarchy", 0, wflags);

	ImGui::Checkbox("Pin Hierarchy Panel", &m_pinHierarchyPanel);
	if (ImGui::Button("New Object"))
	{
		ImGui::OpenPopup("Create GameObject##CreateGameObjectPopup");
	}

	if (ImGui::InputText("Search Object", m_searchNameInputTxt, NAME_INPUT_MAX_LEN,
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

	RenderHierarchyPanelGameObjectsTree(nullptr);

	ShowCreateGameObjectPopup();

Return:
	ImGui::End();
}

void SceneEditorTab::RenderHierarchyPanelGameObjectsTree(GameObject* specified)
{
	ImGui::BeginChild("Child");

	ImGui::Dummy({ 0,5 });

	if (specified == nullptr)
	{
		for (size_t i = 0; i < m_objects.size(); i++)
		{
			auto _obj = m_objects[i];
			RenderHierarchyPanelOf(_obj);
		}
	}
	else
	{
		RenderHierarchyPanelOf(specified);
	}

	ImGui::EndChild();

	if (m_deleteObject)
	{
		if (m_deleteObject->Parent().Get() != nullptr)
		{
			m_deleteObject->RemoveFromParent(true);
		}
		else
		{
			m_scene->RemoveObject(m_deleteObject);
		}
		OnObjectDelete(m_deleteObject);
		m_deleteObject = nullptr;
	}

	if (m_deleteObjectAfterBreakDependencies)
	{
		BreakDependencies(m_deleteObjectAfterBreakDependencies);
		
		if (m_deleteObjectAfterBreakDependencies->Parent().Get() != nullptr)
		{
			m_deleteObjectAfterBreakDependencies->RemoveFromParent(true);
		}
		else
		{
			m_scene->RemoveObject(m_deleteObjectAfterBreakDependencies);
		}
		OnObjectDelete(m_deleteObjectAfterBreakDependencies);
		m_deleteObjectAfterBreakDependencies = nullptr;
	}

	//if (ImGui::BeginDragDropTarget())
	//{
	//	ImGuiDragDropFlags target_flags = 0;
	//	target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
	//	target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
	//	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DND_PAYLOAD", target_flags);
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
		if (specified == nullptr)
		{
			auto dragObj = m_dragingObject;
			if (dragObj->Parent().Get() == nullptr)
			{
				//dragObj->GetScene()->RemoveObject(dragObj);
				auto idx = dragObj->GetComponentRaw<GameObjectEditorComponent>()->id;
				m_objects.erase(m_objects.begin() + idx);
				m_objects.push_back(dragObj);
				ReindexObjects();
			}
			else
			{
				dragObj->RemoveFromParent(true);

				auto mat = dragObj->GetCommittedGlobalTransform();

				Transform transform;
				mat.Decompose(transform.Scale(), transform.Rotation(), transform.Position());

				dragObj->SetLocalTransform(transform);

				m_scene->AddObject(dragObj);
			}
		}

		m_dragingObject = nullptr;
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
}

void SceneEditorTab::RenderInspectorPanel()
{
	if (m_needReloadInspectingObject)
	{
		if (m_inspectingObject)
		{
			ComponentInspector::EndInspectingFor(m_inspectingObject, m_inspectingObjectData, nullptr);
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

		if (dynamic_cast<PhysicsComponent*>(m_removeComp))
		{
			m_inspectingObject->RemoveComponentRaw(dynamic_cast<PhysicsComponent*>(m_removeComp));
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

void SceneEditorTab::ShowCreateComponentPopup()
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
			previewComponentClass = EditorContext::GetInstance()->m_components[m_createComponentContext.selectedComponentId][m_createComponentContext.selectedIdx]->name.c_str();
		}

		if (ImGui::BeginCombo("Component Class", previewComponentClass))
		{
			if (m_createComponentContext.selectedComponentId != INVALID_ID)
			{
				auto& components = EditorContext::GetInstance()->m_components[m_createComponentContext.selectedComponentId];
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
				auto compCtor = EditorContext::GetInstance()->m_components[m_createComponentContext.selectedComponentId][m_createComponentContext.selectedIdx]->ctor;
				auto comp = compCtor();
				switch (m_createComponentContext.selectedComponentId)
				{
				case MainSystemInfo::RENDERING_ID:
					m_inspectingObject->AddComponent(DynamicCast<RenderingComponent>(comp));
					break;
				case MainSystemInfo::PHYSICS_ID:
					m_inspectingObject->AddComponent(DynamicCast<PhysicsComponent>(comp));
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

void SceneEditorTab::ShowCreateGameObjectPopup()
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

		Accessor temp = Accessor::ForString("Path", m_loadObjectFileName, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_loadObjectFileName;
		DataInspector::InspectStringPathEx(nullptr, temp, var, "Load from file", true);

		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 100, ImGui::GetWindowHeight() - 40));

		if (ImGui::Button("OK", ImVec2(100, 0)))
		{
			if (!m_loadObjectFileName.empty())
			{
				auto obj = LoadGameObjectFromFile(m_loadObjectFileName);
				if (obj)
				{
					m_scene->AddObject(obj);
				}
			}
			else
			{
				auto gameObject = mheap::New<GameObject>();
				gameObject->Name() = m_nameInputTxt;
				m_scene->AddObject(gameObject);
			}

			m_loadObjectFileName = "";
			m_nameInputTxt[0] = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine(0, 20);
		if (ImGui::Button("Cancel", ImVec2(100, 0)))
		{
			m_loadObjectFileName = "";
			m_nameInputTxt[0] = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

Handle<GameObject> SceneEditorTab::LoadGameObjectFromFile(const String& path, bool hotReload)
{
	auto ext = FileUtils::GetExtension(path);
	if (ext != "json")
	{
		std::cerr << "[ERROR]: SceneEditorTab::LoadGameObjectFromFile() - invalid file!\n";
		return nullptr;
	}

	Serializer serializer = {};
	serializer.ReadFromFile(path);

	Handle<GameObject> obj;
	serializer.Deserialize(serializer.GetRootUUID(), obj);
	if (obj == nullptr)
	{
		std::cerr << "[ERROR]: SceneEditorTab::LoadGameObjectFromFile() - invalid file!\n";
		return nullptr;
	}

	//m_scene->AddObject(obj);

	if (hotReload)
	{
		m_loadFromFileObject.insert({ obj->GetUUID(), { path,FileSystem::Get()->GetFileModifiedLastTime(path) } });
		obj->PostTraversal([](GameObject* o)
			{
				o->NewComponent<GameObjectEditorComponent>()->hotReloadFromFile = true;
			}
		);
	}

	return obj;
}

Handle<GameObject> SceneEditorTab::LoadStaticModelFromFile(const String& path)
{
	auto ext = FileUtils::GetExtension(path).ToLower();
	if (ext != "fbx" 
		&& ext != "obj"
		&& ext != "dae"
		&& ext != "stl")
	{
		std::cerr << "[ERROR]: SceneEditorTab::LoadStaticModelFromFile() - invalid file!\n";
		return nullptr;
	}

	auto model = resource::Load<Model3D>(path);
	if (!model)
	{
		std::cerr << "[ERROR]: SceneEditorTab::LoadStaticModelFromFile() - invalid file!\n";
		return nullptr;
	}

	auto obj = model->MakeGameObject();
	IndexObject(obj);
	return obj;
}

void SceneEditorTab::ReindexObjects()
{
	size_t i = 0;
	for (auto& o : m_objects)
	{
		o->GetComponentRaw<GameObjectEditorComponent>()->id = i++;
	}
}

void SceneEditorTab::ReindexChildren(Array<Handle<GameObject>>& children)
{
	size_t i = 0;
	for (auto& o : children)
	{
		*(ID*)&o->ParentIdx() = i++;
	}
}

void SceneEditorTab::IndexObject(GameObject* obj)
{
	obj->PostTraversal([](GameObject* o)
		{
			if (!o->HasComponent<GameObjectEditorComponent>())
			{
				o->NewComponent<GameObjectEditorComponent>();
			}
		}
	);
}

void SceneEditorTab::RenderObjectContextPopup(GameObject* obj)
{
	if (ImGui::MenuItem("Expand All"))
	{
		obj->PostTraversal(
			[](GameObject* o)
			{
				o->GetComponentRaw<GameObjectEditorComponent>()->expandAll = true;
			}
		);
	}

	ImGui::Separator();

	if (ImGui::MenuItem(ICON_FA_FILE_IMPORT "  Import GameObject"))
	{
		SystemDialog::FileChooserDialog otp;
		otp.forceInsideResourcesPath = false;
		otp.extensionGroups = {
			{
				"Json files (*.json)",
				{ "json" }
			}
		};

		if (SystemDialog::OpenFileChooser(otp))
		{
			auto newObj = LoadGameObjectFromFile(otp.outputFilePath);
			if (newObj)
			{
				obj->AddChild(newObj);
			}
		}
	}

	if (ImGui::MenuItem(ICON_FA_FILE_IMPORT "  Import Static Model"))
	{
		SystemDialog::FileChooserDialog otp;
		otp.forceInsideResourcesPath = true;
		otp.extensionGroups = {
			{
				"3D Static Model File (*.obj, *.fbx, *.dae, *.stl)",
				{ "obj", "fbx", "dae", "stl" }
			}
		};

		if (SystemDialog::OpenFileChooser(otp))
		{
			auto newObj = LoadStaticModelFromFile(otp.outputFilePath);
			if (newObj)
			{
				obj->AddChild(newObj);
			}
		}
	}

	if (ImGui::MenuItem(ICON_FA_CIRCLE_PLUS "  Add Empty GameObject"))
	{
		obj->AddChild(mheap::New<GameObject>());
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Rename"))
	{
		m_nameInputTxt[0] = 0;
		m_renameObject = obj;
		m_openInputNamePopup = true;
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::MenuItem("Delete"))
	{
		bool allowDelete = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;
		allowDelete = allowDelete && CheckCanBeDeleted(obj);

		if (allowDelete)
		{
			m_deleteObject = obj;
		}
		else
		{
			std::cerr << "[ERROR]: can not delete hot reloaded object's children!\n";
		}
	}

	if (ImGui::MenuItem("Delete (Break All Dependencies)"))
	{
		bool allowDelete = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;
		allowDelete = allowDelete && CheckCanBeDeleted(obj);

		if (allowDelete)
		{
			m_deleteObjectAfterBreakDependencies = obj;
		}
		else
		{
			std::cerr << "[ERROR]: can not delete hot reloaded object's children!\n";
		}
	}

	ImGui::Separator();
	if (ImGui::MenuItem("Switch Kinematic All"))
	{
		bool allow = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;
		if (allow)
		{
			obj->PostTraversal(
				[](GameObject* o)
				{
					if (o->HasComponent<RigidBodyDynamic>())
					{
						o->GetComponentRaw<RigidBodyDynamic>()->SetKinematic(!o->GetComponentRaw<RigidBodyDynamic>()->IsKinematic());
					}
				}
			);
		}
	}

	if (ImGui::MenuItem("Reset Physics Position All"))
	{
		bool allow = obj->Parent().Get() == nullptr || obj->Parent()->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile == false;
		if (allow)
		{
			obj->PostTraversal(
				[](GameObject* o)
				{
					if (o->HasComponent<RigidBodyDynamic>())
					{
						auto local = o->GetLocalTransform();
						auto local2 = local;
						local2.Position().x -= 10.0f;
						o->SetLocalTransform(local2);
						o->SetLocalTransform(local);
					}
				}
			);
		}
	}

	OnRenderGameObjectContextMenu(obj);
}

void SceneEditorTab::BreakDependencies(GameObject* obj)
{
	obj->PostTraversal(
		[](GameObject* o)
		{
			if (o->HasComponent<RigidBody>())
			{
				auto body = o->GetComponentRaw<RigidBody>();
				auto count = body->GetJointsCount();
				for (size_t i = 0; i < count; i++)
				{
					if (!body->GetJoint(i)->IsBroken())
					{
						body->GetJoint(i)->Break();
					}
				}
			}
		}
	);
}

void SceneEditorTab::OnRenderGUI()
{
	RenderHierarchyPanel();
	RenderInspectorPanel();

	ImGui::ShowDemoWindow(0);
}

void SceneEditorTab::OnRenderMenuBar(const String& menuName)
{
	if (menuName == "Tab")
	{
		if (ImGui::MenuItem("Draw Debug", NULL, m_isDrawingDebug))
		{
			m_isDrawingDebug = !m_isDrawingDebug;
		}

		if (ImGui::MenuItem("Draw Selected Object Basis", NULL, m_isDrawingInspectingObjectBasis))
		{
			m_isDrawingInspectingObjectBasis = !m_isDrawingInspectingObjectBasis;
		}

		if (ImGui::MenuItem("Draw Selected Object AABB", NULL, m_isDrawingInspectingObjectAABB))
		{
			m_isDrawingInspectingObjectAABB = !m_isDrawingInspectingObjectAABB;
		}
	}
}

void SceneEditorTab::OnHotReloadGameObject(GameObject* startNewObj, GameObject* startOldObj, GameObject* currentNewObj, GameObject* currentOldObj)
{
	currentNewObj->Name() = currentOldObj->Name();
}

void SceneEditorTab::OnRenderInGameDebugGraphics()
{
	//#ifdef _DEBUG
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics) return;

	if (m_isDrawingInspectingObjectBasis && m_inspectingObject)
	{
		auto& mat = m_inspectingObject->GetCommittedGlobalTransform();

		debugGraphics->DrawRay(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
		debugGraphics->DrawRay(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
		debugGraphics->DrawRay(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });

		/*auto physicsComp = m_inspectingObject->GetComponentRaw<PhysicsComponent>();
		if (physicsComp)
		{
			physicsComp->OnDrawDebug();
		}*/
	}

	if (m_isDrawingInspectingObjectAABB && m_inspectingObject)
	{
		m_inspectingObject->GetRoot()->PostTraversal(
			[debugGraphics](GameObject* o)
			{
				if (o->HasComponent<RenderingComponent>())
				{
					debugGraphics->DrawAABox(o->GetComponentRaw<RenderingComponent>()->GetGlobalAABB());
				}
			}
		);
	}

	if (m_isDrawingDebug)
	{
		for (auto& obj : m_objects)
		{
			obj->PostTraversal([](GameObject* obj)
				{
					auto physicsComp = obj->GetComponentRaw<PhysicsComponent>();
					if (physicsComp)
					{
						physicsComp->OnDrawDebug();
					}

					auto renderingComp = obj->GetComponentRaw<RenderingComponent>();
					if (renderingComp)
					{
						renderingComp->OnDrawDebug();
					}

					auto animationComp = obj->GetComponentRaw<AnimationComponent>();
					if (animationComp)
					{
						animationComp->OnDrawDebug();
					}
				}
			);
		}
	}
	
	//debugGraphics->DrawDirection(Vec3(-10, 0, 0), Vec3(20, 0, 0), { 1,0,0,1 }, { 1,0,0,1 });
	//debugGraphics->DrawDirection(Vec3(0, -10, 0), Vec3(0, 20, 0), { 0,1,0,1 }, { 0,1,0,1 });
	//debugGraphics->DrawDirection(Vec3(0, 0, -10), Vec3(0, 0, 20), { 0,0,1,1 }, { 0,0,1,1 });


	//// test
	//debugGraphics->DrawSphere({ Vec3(0, 0, 0), 1 }, Vec4(1, 0, 0, 1));
	//debugGraphics->DrawDirection(Vec3(0, 0, 0.5f), Vec3::X_AXIS, { 0,0,1,1 }, { 0,1,0,1 });

	//debugGraphics->DrawCapsule(Capsule(Vec3(0, 0, 0), 2.0f, 1.0f), Vec4(1, 0, 0, 1));
	//debugGraphics->DrawRay(Vec3(0, 0, 0), Capsule::DEFAULT_UP_AXIS, { 0,0,1,1 }, { 0,1,0,1 });
	//debugGraphics->DrawRay(Vec3(0, 1, 0), Vec3::X_AXIS, { 0,0,1,1 }, { 0,1,0,1 });

	//debugGraphics->DrawCapsule(Capsule(Vec3(1, 1, 1), Vec3(5, 0, -5), 5.0f, 2.0f), Vec4(1, 0, 0, 1));
	//debugGraphics->DrawDirection(Vec3(5, 0, -5), Vec3::X_AXIS * 2.5f, { 0,0,1,1 }, { 0,1,0,1 });
	//debugGraphics->DrawDirection(Vec3(5, 0, -5), Vec3::Y_AXIS * 2.0f, { 0,0,1,1 }, { 0,1,0,1 });

	/*{
		static size_t temp = 0;
		for (size_t j = 0; j < 10; j++)
		{
			if (temp > 300)
			{
				temp = 0;
			}

			size_t numSamples = (temp++) / 30 + 10;
			auto center = Vec3(5, 0, -5 * ((float)j + 1));
			float radius = 4.f;
			float arc = 0;
			float dArc = 2 * PI / (float)numSamples;
			std::vector<Vec3> vertices;
			for (size_t i = 0; i < numSamples; i++)
			{
				auto v1 = Vec3(std::sin(arc), 0, std::cos(arc)) * radius + center;

				arc += dArc;

				auto v2 = Vec3(std::sin(arc), 0, std::cos(arc)) * radius + center;

				vertices.push_back(center);
				vertices.push_back(v1);
				vertices.push_back(v2);
			}
			debugGraphics->DrawMesh(vertices.data(), vertices.size(), {}, Vec4(1, 0, 0, 1));
		}

	}*/

	//#endif // _DEBUG
}

void SceneEditorTab::Inspect(ClassMetadata* metaData)
{
	size_t currentDepth = -1;
	metaData->ForEachProperties(
		[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
		{
			if (depth == 0)
			{
				/*ImGui::SameLine();
				if (ImGui::Button(ICON_FA_ROTATE " Synch Transform Recursive"))
				{
					
				}*/

				ImGui::SetNextItemOpen(true);

			}

			bool rawInspect = true;
			if (currentDepth + 1 == depth)
			{
				Handle<SceneEditorComponentData> editorCompData = metadata->GenericDictionary()->Get<SceneEditorComponentData>("SceneEditorComponentData");
				if (!editorCompData)
				{
					editorCompData = mheap::New<SceneEditorComponentData>();
					metadata->GenericDictionary()->Store("SceneEditorComponentData", editorCompData);
				}

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 5.f));
				auto open = ImGui::TreeNodeEx(metadata->GetName(), ImGuiTreeNodeFlags_FramePadding);
				ImGui::PopStyleVar();

				if (depth == 0)
				{
					if (ImGui::Button(ICON_FA_ROTATE " Synch Transform"))
					{
						auto& globalTransform = m_inspectingObject->GetCommittedGlobalTransform();
						m_inspectingObject->SetGlobalTransform(globalTransform, INVALID_ID, GameObject::TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL, true);
						//auto parentTransform = m_inspectingObject->Parent().Get() ? m_inspectingObject->Parent()->GetCommittedGlobalTransform() : Mat4::Identity();
						//auto local = globalTransform * parentTransform.GetInverse();
						//m_inspectingObject->SetLocalTransform(Transform::FromTransformMatrix(local),)
					}
				}

				if (editorCompData->lastOpen != open)
				{
					auto comp = dynamic_cast<MainComponent*>(metadata->GetInstance());
					if (comp)
					{
						if (open)
						{
							// open collapsing header
							ComponentInspector::BeginInspectingFor(comp->GetGameObject(), metadata, comp);
						}
						else
						{
							// close collapsing header
							ComponentInspector::EndInspectingFor(comp->GetGameObject(), metadata, comp);
						}
					}
				}
				editorCompData->lastOpen = open;

				m_inspectPropertiesIsOpenStack.push_back(open);
				m_inspectInlinePropertiesCountStack.push_back(metadata->GetInlinePropertiesCount());
				currentDepth++;

				if (depth != 0 && dynamic_cast<MainComponent*>(metadata->GetInstance()))
				{
					//ImGui::SameLine();
					auto pos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 50, pos.y - 35));
					String btnName = String(ICON_FA_TRASH) + "##" + metadata->GetName();
					if (ImGui::Button(btnName.c_str(), ImVec2(30, 30)))
					{
						m_removeComp = dynamic_cast<MainComponent*>(metadata->GetInstance());
						ComponentInspector::EndInspectingFor(m_removeComp->GetGameObject(), metadata, m_removeComp);
					}

					ImGui::SetCursorPos(pos);
				}

				if (open)// && propertyName)
				{
					if (ComponentInspector::Get()->Inspect(0, metadata->GetInstance(), metadata, propertyName))
					{
						rawInspect = false;
					}
				}

				m_inspectPropertiesIsRawInspectStack.push_back(rawInspect);
			}

			bool open = m_inspectPropertiesIsOpenStack.back();
			if (open && m_inspectPropertiesIsRawInspectStack.back() && propertyName)
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

void SceneEditorTab::OnShow()
{
	m_onSaveListenerId = EditorContext::Get()->EventDispatcher()->AddListener(EditorContext::EVENT::MENU_ON_SAVE,
		[](EditorContext* ctx, int argc, void** argv, ID id)
		{
			auto self = (SceneEditorTab*)id;
			auto path = self->GetSaveFilePath();

			SceneEditorSaveData data(self);
			
			Serializer serializer = {};
			serializer.Serialize(self->m_scene);
			serializer.SetRootUUID(self->m_scene->GetUUID(), 0);

			serializer.Serialize(&data);
			serializer.SetRootUUID(data.GetUUID(), 1);

			serializer.WriteToFile(path);
		},
		ID(this)
	);
}

void SceneEditorTab::OnHide()
{
	if (m_onSaveListenerId != INVALID_ID)
	{
		//auto d = EditorContext::Get()->EventDispatcher();
		EditorContext::Get()->EventDispatcher()->RemoveListener(m_onSaveListenerId);
		m_onSaveListenerId = INVALID_ID;
	}
}

void SceneEditorTab::OnOpen()
{
#ifdef PLUGIN_ALLOW_HOT_RELOAD
	m_scriptsHotReloadListenerIdBegin = Runtime::Get()->EventDispatcher()->AddListener(Runtime::EVENT_HOT_RELOAD_SCRIPTS_BEGIN,
		[](Runtime* runtime, int argc, void** argv, ID editorId)
		{
			auto self = (SceneEditorTab*)editorId;
			self->m_inspectingObjectData = nullptr;
		},
		ID(this)
	);

	m_scriptsHotReloadListenerIdEnd = Runtime::Get()->EventDispatcher()->AddListener(Runtime::EVENT_HOT_RELOAD_SCRIPTS_END,
		[](Runtime* runtime, int argc, void** argv, ID editorId)
		{
			auto self = (SceneEditorTab*)editorId;
			if (self->m_inspectingObject)
			{
				self->m_inspectingObjectData = self->m_inspectingObject->GetMetadata(0);
			}
		},
		ID(this)
	);
#endif // PLUGIN_ALLOW_HOT_RELOAD
}
void SceneEditorTab::OnClose()
{

#ifdef PLUGIN_ALLOW_HOT_RELOAD
	Runtime::Get()->EventDispatcher()->RemoveListener(m_scriptsHotReloadListenerIdBegin);
	Runtime::Get()->EventDispatcher()->RemoveListener(m_scriptsHotReloadListenerIdEnd);
#endif // PLUGIN_ALLOW_HOT_RELOAD
}

void SceneEditorTab::OnObjectDelete(GameObject* obj)
{
	if (m_inspectingObject && (obj->GetRoot() == m_inspectingObject->GetRoot()))
	{
		bool has = false;
		obj->PostTraversal(
			[&](GameObject* o)
			{
				if (o == m_inspectingObject)
				{
					has = true;
				}
			}
		);

		if (has)
		{
			OnObjectSelected(nullptr);
		}
	}

	obj->PostTraversal(
		[&](GameObject* o) 
		{
			auto it = m_loadFromFileObject.find(o->GetUUID());
			if (it != m_loadFromFileObject.end())
			{
				m_loadFromFileObject.erase(it);
			}
		}
	);
}

void SceneEditorTab::AddObjectToEditor(GameObject* obj)
{
	if (obj->Parent().Get() != nullptr)
	{
		return;
	}

	IndexObject(obj);

	auto editorComp = obj->GetComponentRaw<GameObjectEditorComponent>();
	editorComp->id = m_objects.size();
	m_objects.push_back(obj);
}

void SceneEditorTab::ReloadCurrentInspectingObject()
{
	m_needReloadInspectingObject = true;
}

void SceneEditorTab::WriteSaveDataToJson(Serializer* serializer, json& j)
{
	{
		auto arr = json::array();
		for (const auto& [uuid, data] : m_loadFromFileObject)
		{
			json temp;
			temp["UUID"] = uuid;
			temp["FilePath"] = data.filePath;
			temp["LastModifiedTime"] = data.loadedLastModifiedTime;
			arr.push_back(temp);
		}

		j["LoadedFromFileObject"] = arr;
	}

	{
		auto arr = json::array();
		for (auto& o : m_objects)
		{
			arr.push_back(json(serializer->Serialize(o)));
		}

		j["Objects"] = arr;
	}

	{
		j["InspectingObject"] = serializer->Serialize(m_inspectingObject);
		j["DrawDebug"] = m_isDrawingDebug;
		j["DrawingInspectingObjectBasis"] = m_isDrawingInspectingObjectBasis;
		j["DrawingInspectingObjectAABB"] = m_isDrawingInspectingObjectAABB;
	}
}

void SceneEditorTab::ReadSaveDataFromJson(Serializer* serializer, const json& j)
{
	if (j.contains("LoadedFromFileObject"))
	{
		m_isHotDeserializingGameObjectFromFile = true;

		auto& arr = j["LoadedFromFileObject"];
		Handle<GameObject> object = nullptr;
		for (size_t i = 0; i < arr.size(); i++)
		{
			auto& temp = arr[i];
			UUID uuid = temp["UUID"];
			String filePath = temp["FilePath"];
			size_t lastModifiedTime = temp["LastModifiedTime"];

			object = nullptr;
			serializer->Deserialize(uuid, object);

			if (object && lastModifiedTime != FileSystem::Get()->GetFileModifiedLastTime(filePath))
			{
				auto replaceObject = LoadGameObjectFromFile(filePath);
				m_loadFromFileObject[replaceObject->GetUUID()].loadedLastModifiedTime = lastModifiedTime;
				assert(replaceObject.Get() != nullptr);

				std::vector<GameObject*> stack0;
				std::vector<GameObject*> stack1;
				
				{
					bool _break = false;
					stack0.push_back(object);
					stack1.push_back(replaceObject);
					while (!stack0.empty() && !_break)
					{
						if (stack0.size() != stack1.size())
						{
							std::cout << "[WARN]: GameObject structure modified!\n";
							_break = true;
							break;
						}

						auto o0 = stack0.back();
						auto o1 = stack1.back();
						stack0.pop_back();
						stack1.pop_back();

						o1->CopyTransform(o0);

						OnHotReloadGameObject(replaceObject, object, o1, o0);

						if (o0->Children().size() != o1->Children().size())
						{
							std::cout << "[WARN]: GameObject structure modified!\n";
							_break = true;
							break;
						}

						for (size_t i = 0; i < o0->Children().size(); i++)
						{
							stack0.push_back(o0->Children()[i]);
							stack1.push_back(o1->Children()[i]);
						}
					}
				}

				if (object->Parent().Get() == nullptr)
				{
					auto editorId = object->GetComponentRaw<GameObjectEditorComponent>()->id;
					m_objects[editorId] = replaceObject.Get();
					AddObjectToEditor(replaceObject);

					m_scene->RemoveObject(object);
					m_scene->AddObject(replaceObject);
				}
				else
				{
					replaceObject->PostTraversal([](GameObject* o)
						{
							if (!o->HasComponent<GameObjectEditorComponent>())
							{
								o->NewComponent<GameObjectEditorComponent>();
							}
						}
					);

					auto parent = object->Parent().Get();
					auto idx = object->ParentIdx();
					object->RemoveFromParent(true);
					parent->AddChild(replaceObject, idx);
				}
			}
			else
			{
				LoadedObjectFromFileData data;
				data.filePath = filePath;
				data.loadedLastModifiedTime = lastModifiedTime;
				m_loadFromFileObject.insert({ object->GetUUID(),data });

				object->PostTraversal([](GameObject* o)
					{
						if (!o->HasComponent<GameObjectEditorComponent>())
						{
							o->NewComponent<GameObjectEditorComponent>();
						}

						o->GetComponentRaw<GameObjectEditorComponent>()->hotReloadFromFile = true;
					}
				);
			}
		}

		m_isHotDeserializingGameObjectFromFile = false;
	}

	if (j.contains("Objects"))
	{
		m_objects.clear();

		Handle<GameObject> obj;
		auto& arr = j["Objects"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			serializer->Deserialize(arr[i], obj);
			m_objects.push_back(obj);
		}

		ReindexObjects();
	}

	if (j.contains("InspectingObject"))
	{
		Handle<GameObject> inspectingObject;
		serializer->Deserialize(j["InspectingObject"], inspectingObject);
		OnObjectSelected(inspectingObject);
	}

	if (j.contains("DrawDebug"))
	{
		m_isDrawingDebug = j["DrawDebug"];
	}

	if (j.contains("DrawingInspectingObjectBasis"))
	{
		m_isDrawingInspectingObjectBasis = j["DrawingInspectingObjectBasis"];
	}

	if (j.contains("DrawingInspectingObjectAABB"))
	{
		m_isDrawingInspectingObjectAABB = j["DrawingInspectingObjectAABB"];
	}
}

void SceneEditorTab::OnRenderGameObjectContextMenu(GameObject* obj)
{
}

bool SceneEditorTab::CheckCanBeDeleted(GameObject* obj)
{
	return true;
}

void SceneEditorTab::HighlightObject(GameObject* obj)
{
	auto it = m_highlightingObjects.find(obj);
	if (it != m_highlightingObjects.end())
	{
		return;
	}

	m_highlightingObjects.insert(obj);
}

void SceneEditorTab::UnhighlightObject(GameObject* obj)
{
	auto it = m_highlightingObjects.find(obj);
	if (it == m_highlightingObjects.end())
	{
		return;
	}

	m_highlightingObjects.erase(it);
}
