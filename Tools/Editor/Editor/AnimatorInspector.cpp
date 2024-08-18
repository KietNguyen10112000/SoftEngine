#include "AnimatorInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "AnimatorEditorTab.h"
#include "SceneEditorTab.h"
#include "EditorFont.h"

AnimatorInspector::AnimatorInspector(AnimatorSkeletalArray* animator, ClassMetadata* meta) 
	: ComponentInspectorBase(animator), m_animator(animator), m_metadata(meta)
{
	m_tposeLayer = AnimatorEditorTab::MakeTPoseLayer(m_animator);
	BuildModelHierarchy();
}

AnimatorInspector::~AnimatorInspector()
{
	for (auto& n : m_modelNodes)
	{
		delete n;
	}
	m_modelNodes.clear();
}

void AnimatorInspector::OnBeginInspecting()
{
	auto root = m_animator->GetGameObject()->GetRoot();
	SetOpacityForObject(root, m_currentAlpha);
}

void AnimatorInspector::OnEndInspecting()
{
	auto root = m_animator->GetGameObject()->GetRoot();
	SetOpacityForObject(root, 1.0f);
}

void AnimatorInspector::Inspect()
{
	if (m_isEnableTPose > 1)
	{
		m_tposeLayer->Run(0);
		m_animator->UpdateDataToRenderer(m_animator->GetGameObject()->GetScene(), m_tposeLayer);
		m_isEnableTPose--;
	}

	{
		// adjust opacity to edit shapes
		if (ImGui::SliderFloat("Opacity", &m_currentAlpha, 0.0f, 1.0f))
		{
			auto root = m_animator->GetGameObject()->GetRoot();
			SetOpacityForObject(root, m_currentAlpha);
		}

		bool enableTPose = m_isEnableTPose != 0;
		if (ImGui::Checkbox("TPose", &enableTPose))
		{
			m_animator->SetRunning(!enableTPose);
			if (enableTPose)
			{
				m_tposeLayer->Run(0);
				m_isEnableTPose = 5;
			}
			else
			{
				m_isEnableTPose = 0;
			}
		}
	}
	
	ImGuiTreeNodeFlags nodeFlags =
		ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_AllowItemOverlap
		//| ImGuiTreeNodeFlags_FramePadding
		| ImGuiTreeNodeFlags_Framed;
	auto open = ImGui::TreeNodeEx("Skeleton", nodeFlags);

	if (open)
	{
		RenderModelNodeHierarchy(nullptr, nullptr);
		ImGui::TreePop();
	}

	ImGui::Dummy({ 25,50 });
}

void AnimatorInspector::BuildModelHierarchy()
{
	auto& nodes = m_animator->m_model3D->m_nodes;

	std::vector<ModelNode*> modelNodes;
	for (size_t i = 0; i < nodes.size(); i++)
	{
		auto& node = nodes[i];
		auto modelNode = new ModelNode();
		modelNodes.push_back(modelNode);

		modelNode->nodeIdx = i;

		if (node.parentId != INVALID_ID)
		{
			modelNodes[node.parentId]->children.push_back(modelNode);
			modelNode->parent = modelNodes[node.parentId];
		}
		else
		{
			assert(i == 0);
		}
	}

	m_root = modelNodes[0];
	m_modelNodes.swap(modelNodes);

	m_boundObjects.Resize(m_modelNodes.size());
}

void AnimatorInspector::RenderModelNodeHierarchy(void (*callback)(ModelNode*, void*), void* userPtr)
{
	auto& model = m_animator->m_model3D;
	auto& nodes = model->m_nodes;

	ModelNode* rootBone = m_root;
	//if (!m_isShowRootNode)
	{
		for (auto& node : m_modelNodes)
		{
			if (nodes[node->nodeIdx].boneId != INVALID_ID)
			{
				rootBone = node;
				break;
			}
		}
	}

	if (rootBone)
		RenderModelNodeHierarchyImpl(callback, userPtr, rootBone, nullptr);
}

void AnimatorInspector::RenderModelNodeHierarchyImpl(void (*callback)(ModelNode*, void*), void* userPtr, ModelNode* modelNode, void* outRect)
{
	auto& model = m_animator->m_model3D;
	auto& nodes = model->m_nodes;
	auto& node = nodes[modelNode->nodeIdx];

	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_SpanAvailWidth
		| ImGuiTreeNodeFlags_AllowItemOverlap
		| ImGuiTreeNodeFlags_SpanFullWidth
		| (modelNode->isTryingExpand ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None)
		| (modelNode->children.empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None)
		| (modelNode->isSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None)
		| ImGuiTreeNodeFlags_NavLeftJumpsBackHere
		;

	String name = "<Unnamed>";
	if (node.boneId != INVALID_ID)
	{
		name = model->m_boneNames[node.boneId];
	}
	ImGui::PushID(ID(modelNode->nodeIdx));
	auto open = ImGui::TreeNodeEx((void*)modelNode, nodeFlags, name.c_str());
	ImGui::PopID();

	if (ImGui::BeginDragDropTarget())
	{
		ImGuiDragDropFlags target_flags = 0;
		target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;
		//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DND_PAYLOAD", target_flags);
		if (payload && ImGui::IsMouseReleased(0))
		{
			auto dragObj = *(GameObject**)payload->Data;
			m_boundObjects[modelNode->nodeIdx] = dragObj;
		}
	}

	const ImRect nodeRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

	if (ImGui::IsItemClicked() && open == modelNode->isOpen)
	{
		if (!open)
		{
			modelNode->ForEach([](ModelNode* n) { n->isSelected = true; });
		}
		else
		{
			modelNode->isSelected = true;
		}

		if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightShift))
		{
			size_t i = 0;
			size_t start = INVALID_ID;
			size_t end = INVALID_ID;
			for (auto& n : m_modelNodes)
			{
				if (n->isSelected)
				{
					if (start == INVALID_ID)
					{
						start = i;
					}

					end = i;
				}
				i++;
			}

			if (start != INVALID_ID && end != INVALID_ID && end > start)
			{
				for (i = start; i <= end; i++)
				{
					m_modelNodes[i]->isSelected = true;
				}
			}
		}
		else if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightCtrl))
		{

		}
		else
		{
			for (auto& n : m_modelNodes)
			{
				n->isSelected = false;
			}
		}

		if (!open)
		{
			modelNode->ForEach([](ModelNode* n) { n->isSelected = true; });
		}
		else
		{
			modelNode->isSelected = true;
		}
	}

	if (callback)
	{
		ImGui::SameLine(); callback(modelNode, userPtr); ImGui::Dummy({ 0, 0 });
	}

	if (m_boundObjects[modelNode->nodeIdx])
	{
		auto boundObject = m_boundObjects[modelNode->nodeIdx].Get();
		ImGui::SameLine(0, 20);
		ImGui::PushID(ID(modelNode->nodeIdx));
		ImGui::PushFont(EditorFont::Get()->GetFont(18));
		ImGui::Button(ICON_FA_TAG);
		ImGui::PopFont();
		ImGui::PopID();

		auto currentSceneEditorTab = dynamic_cast<SceneEditorTab*>(EditorContext::Get()->GetCurrentTab());
		if (ImGui::IsItemHovered())
		{
			if (ImGui::BeginTooltip())
			{
				auto& text = boundObject->Name();
				ImGui::TextUnformatted(text.empty() ? "<Unnamed>" : text.c_str());
				ImGui::EndTooltip();
			}

			if (!modelNode->isHovering && currentSceneEditorTab)
			{
				currentSceneEditorTab->HighlightObject(boundObject);
			}

			modelNode->isHovering = true;
		}
		else
		{
			if (modelNode->isHovering && currentSceneEditorTab)
			{
				currentSceneEditorTab->UnhighlightObject(boundObject);
			}

			modelNode->isHovering = false;
		}
	}

	modelNode->isOpen = open;

	if (open)
	{
		const ImColor TreeLineColor = ImColor(128, 128, 128, 255);// ImGui::GetColorU32(ImGuiCol_Text);
		const float SmallOffsetX = -6.0f; //for now, a hardcoded value; should take into account tree indent size
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
		verticalLineStart.x += SmallOffsetX; //to nicely line up with the arrow symbol
		ImVec2 verticalLineEnd = verticalLineStart;

		verticalLineStart.y -= 8;

		for (auto& child : modelNode->children)
		{
			ImRect childRect;
			const float HorizontalTreeLineSize = 16.0f; //chosen arbitrarily

			RenderModelNodeHierarchyImpl(callback, userPtr, child, &childRect);

			const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
			drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
			verticalLineEnd.y = midpoint;
		}

		if (!modelNode->children.empty())
		{
			drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);
		}

		ImGui::TreePop();
	}

	if (outRect)
	{
		*(ImRect*)outRect = nodeRect;
	}
}