#include "AnimatorInspector.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeSphere.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"
#include "MainSystem/Physics/Joints/D6Joint.h"
#include "MainSystem/Physics/Joints/FixedJoint.h"
#include "MainSystem/Physics/Materials/PhysicsMaterial.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "Graphics/DebugGraphics.h"
#include "Graphics/Graphics.h"

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
	DrawDebugSkeleton();

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
		if (ImGui::Button(ICON_FA_GEARS "  Make RigidBody Skeleton"))
		{
			MakeRigidBodySkeleton();
		}

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

void AnimatorInspector::MakeRigidBodySkeleton()
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

	auto& boneGlobals = m_tposeLayer->NodeGlobalTransforms();
	auto& offsets = m_animator->m_model3D->m_boneOffsetMatrixs;
	auto& objGlobal = m_animator->GetGameObject()->GetCommittedGlobalTransform();

	std::vector<Vec3> nodePositions;
	nodePositions.resize(nodes.size());

	for (size_t i = 0; i < nodes.size(); i++)
	{
		auto& node = nodes[i];
		if (node.boneId != INVALID_ID)
		{
			nodePositions[i] = (offsets[node.boneId].GetInverse() * objGlobal).Position();
		}
		else
		{
			nodePositions[i] = objGlobal.Position();
		}
	}

	constexpr float SPHERE_RADIUS = 0.002f; // 
	constexpr float CAPSULE_RADIUS = 0.01f; // 
	constexpr float USE_OVERLAP_BONE = 0; // allow bone head and tail to be overlapped with its parent and children

	auto material = std::make_shared<PhysicsMaterial>(0.5f, 0.5f, 0.5f);
	Array<Handle<GameObject>> boneRigidBodies;
	boneRigidBodies.Resize(nodes.size());

	static void (*fn)(RigidBodyDynamic*, ModelNode*, Array<Handle<GameObject>>&,
		const SharedPtr<PhysicsMaterial>&, std::vector<Vec3>&, AnimModel*)
		= [](RigidBodyDynamic* parent, ModelNode* modelNode, Array<Handle<GameObject>>& objects,
			const SharedPtr<PhysicsMaterial>& material, std::vector<Vec3>& nodePositions, AnimModel* model) -> void
	{
		if (modelNode->children.size() == 0)
		{
			return;
		}

		{
			size_t count = 0;
			for (auto& child : modelNode->children)
			{
				if (model->m_nodes[child->nodeIdx].boneId == INVALID_ID)
				{
					count++;
				}
			}
			if (count == modelNode->children.size())
			{
				return;
			}
		}

		auto& jointPos = nodePositions[modelNode->nodeIdx];
		Mat4 objGLobalTransform;
		if (parent)
		{
			auto& parentTransform = parent->GetCurrentObject()->GetCommittedGlobalTransform();
			auto X = (jointPos - parentTransform.Position()).Normal();
			auto u = Vec3::X_AXIS;
			if (std::abs(X.Dot(u)) < 0.0001f)
			{
				u = Vec3::UP;
			}
			auto Y = X.Cross(u).Normal();
			auto Z = X.Cross(Y).Normal();
			objGLobalTransform = Mat4(
				Vec4(X, 0.0f),
				Vec4(Y, 0.0f),
				Vec4(Z, 0.0f),
				Vec4(jointPos, 1.0f)
			);
		}
		else
		{
			objGLobalTransform = Mat4(
				Vec4(Vec3::X_AXIS, 0.0f),
				Vec4(Vec3::Y_AXIS, 0.0f),
				Vec4(Vec3::Z_AXIS, 0.0f),
				Vec4(jointPos, 1.0f)
			);
		}

		auto obj = mheap::New<GameObject>();
		obj->Name() = model->m_boneNames[model->m_nodes[modelNode->nodeIdx].boneId];
		objects[modelNode->nodeIdx] = obj;

		auto dynamic = obj->NewComponent<RigidBodyDynamic>();
		obj->SetGlobalTransform(objGLobalTransform, INVALID_ID, GameObject::TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL);

		for (auto& child : modelNode->children)
		{
			if (model->m_nodes[child->nodeIdx].boneId == INVALID_ID)
			{
				continue;
			}

			auto length = (obj->GetCommittedGlobalTransform().Position() - nodePositions[child->nodeIdx]).Length();
			auto capsule = std::make_shared<PhysicsShapeCapsule>(
				std::max(length - CAPSULE_RADIUS * 2.0f * (1.0f - USE_OVERLAP_BONE), 0.001f), CAPSULE_RADIUS, material);

			auto X = (nodePositions[child->nodeIdx] - obj->GetCommittedGlobalTransform().Position()).Normal();
			auto u = Vec3::X_AXIS;
			if (std::abs(X.Dot(u)) < 0.0001f)
			{
				u = Vec3::UP;
			}
			auto Y = X.Cross(u).Normal();
			auto Z = X.Cross(Y).Normal();

			capsule->SetLocalTransform(Transform::FromTransformMatrix(Mat4(
				Vec4(X, 0.0f),
				Vec4(Y, 0.0f),
				Vec4(Z, 0.0f),
				Vec4((nodePositions[child->nodeIdx] + obj->GetCommittedGlobalTransform().Position()) / 2.0f, 1.0f)
			) * obj->GetCommittedGlobalTransform().GetInverse()));
			dynamic->AddShape(capsule);

			fn(dynamic, child, objects, material, nodePositions, model);

			auto& childObj = objects[child->nodeIdx];
			if (childObj)
			{
				obj->AddChild(childObj);
			}
		}

		dynamic->SetKinematic(true);
		dynamic->SetDensity(1000);

		if (parent)
		{
			auto& parentTransform = parent->GetCurrentObject()->GetCommittedGlobalTransform();
			auto& jointGlobalTransformMat = objGLobalTransform;
			Transform localframe0 = Transform::FromTransformMatrix(jointGlobalTransformMat * parentTransform.GetInverse());
			Transform localframe1 = {};
			auto d6Joint = mheap::New<D6Joint>(parent, localframe0, dynamic, localframe1);
			d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Y, D6Joint::MOTION_TYPE::FREE);
			d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Z, D6Joint::MOTION_TYPE::FREE);
		}
	};

	fn(nullptr, rootBone, boneRigidBodies, material, nodePositions, model);
	auto newObj = mheap::New<GameObject>();
	newObj->AddChild(boneRigidBodies[rootBone->nodeIdx]);

	//for (size_t i = 0; i < nodes.size(); i++)
	//{
	//	auto& node = nodes[i];
	//	auto& nodeGlobal = nodeGlobals[i];
	//	auto& modelNode = m_modelNodes[i];

	//	if (node.boneId != INVALID_ID)
	//	{
	//		auto obj = mheap::New<GameObject>();
	//		auto dynamic = obj->NewComponent<RigidBodyDynamic>();

	//		for (auto& child : modelNode->children)
	//		{
	//			auto capsule = std::make_shared<PhysicsShapeSphere>(, CAPSULE_RADIUS, material);

	//		}

	//		dynamic->SetKinematic(true);
	//		dynamic->SetDensity(1000);
	//		obj->SetGlobalTransform(Mat4::Translation(nodeGlobal.Position()));
	//		obj->Name() = model->m_boneNames[node.boneId];

	//		boneRigidBodies[i] = obj;

	//		if (newObj->Children().Size() == 0 && node.parentId != INVALID_ID)
	//		{
	//			newObj->AddChild(obj);
	//		}

	//		if (node.parentId != INVALID_ID && nodes[node.parentId].boneId != INVALID_ID)
	//		{
	//			auto& parentObj = boneRigidBodies[node.parentId];
	//			auto& parentNodeGlobal = parentObj->GetCommittedGlobalTransform();
	//			assert(parentObj != nullptr);

	//			parentObj->AddChild(obj);

	//			auto X = (nodeGlobal.Position() - parentNodeGlobal.Position()).Normal();
	//			auto u = Vec3::X_AXIS;
	//			if (std::abs(X.Dot(u)) < 0.0001f)
	//			{
	//				u = Vec3::UP;
	//			}
	//			auto Y = X.Cross(u).Normal();
	//			auto Z = X.Cross(Y).Normal();

	//			obj->SetGlobalTransform(Mat4(
	//				Vec4(X, 0.0f),
	//				Vec4(Y, 0.0f),
	//				Vec4(Z, 0.0f),
	//				Vec4(nodeGlobal.Position(), 1.0f)
	//			), INVALID_ID, GameObject::TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL);

	//			auto length = (parentNodeGlobal.Position() - nodeGlobal.Position()).Length();
	//			if (length > SPHERE_RADIUS * 2.0f + CAPSULE_RADIUS * 2.0f + 0.01f)
	//			{
	//				auto boneLength = length - SPHERE_RADIUS * 2.0f - 0.005f;

	//				auto bone = mheap::New<GameObject>();
	//				auto capsule = std::make_shared<PhysicsShapeCapsule>(boneLength - CAPSULE_RADIUS * 2.0f, CAPSULE_RADIUS, material);
	//				auto boneDynamic = bone->NewComponent<RigidBodyDynamic>(capsule);
	//				boneDynamic->SetKinematic(true);
	//				boneDynamic->SetDensity(1000);

	//				parentObj->AddChild(bone);

	//				auto bonePosition = (parentNodeGlobal.Position() + nodeGlobal.Position()) / 2.0f;

	//				auto boneGlobalTransformMat = Mat4(
	//					Vec4(X, 0.0f),
	//					Vec4(Y, 0.0f),
	//					Vec4(Z, 0.0f),
	//					Vec4(bonePosition, 1.0f)
	//				);

	//				// set joint to parent
	//				{
	//					auto& jointPosition = parentNodeGlobal.Position();
	//					auto jointGlobalTransformMat = Mat4(
	//						Vec4(X, 0.0f),
	//						Vec4(Y, 0.0f),
	//						Vec4(Z, 0.0f),
	//						Vec4(jointPosition, 1.0f)
	//					);

	//					auto localframe0 = Transform::FromTransformMatrix(jointGlobalTransformMat * parentNodeGlobal.GetInverse());
	//					auto localframe1 = Transform::FromTransformMatrix(jointGlobalTransformMat * boneGlobalTransformMat.GetInverse());

	//					bone->SetGlobalTransform(boneGlobalTransformMat, INVALID_ID, GameObject::TRANSFORM_CONSTRAINT::GLOBAL_TO_LOCAL);

	//					auto d6Joint = mheap::New<D6Joint>(parentObj->GetComponent<RigidBodyDynamic>(), localframe0, boneDynamic, localframe1);
	//					d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Y, D6Joint::MOTION_TYPE::FREE);
	//					d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Z, D6Joint::MOTION_TYPE::FREE);
	//				}

	//				// set up joint to current
	//				{
	//					auto& jointPosition = nodeGlobal.Position();
	//					auto jointGlobalTransformMat = Mat4(
	//						Vec4(X, 0.0f),
	//						Vec4(Y, 0.0f),
	//						Vec4(Z, 0.0f),
	//						Vec4(jointPosition, 1.0f)
	//					);

	//					auto localframe0 = Transform::FromTransformMatrix(jointGlobalTransformMat * obj->GetCommittedGlobalTransform().GetInverse());
	//					auto localframe1 = Transform::FromTransformMatrix(jointGlobalTransformMat * boneGlobalTransformMat.GetInverse());

	//					mheap::New<FixedJoint>(dynamic, localframe0, boneDynamic, localframe1);
	//				}
	//			}
	//			else
	//			{
	//				auto boneGlobalTransformMat = Mat4(
	//					Vec4(X, 0.0f),
	//					Vec4(Y, 0.0f),
	//					Vec4(Z, 0.0f),
	//					Vec4(nodeGlobal.Position(), 1.0f)
	//				);

	//				auto& jointPosition = parentNodeGlobal.Position();
	//				auto jointGlobalTransformMat = Mat4(
	//					Vec4(X, 0.0f),
	//					Vec4(Y, 0.0f),
	//					Vec4(Z, 0.0f),
	//					Vec4(jointPosition, 1.0f)
	//				);
	//				auto localframe0 = Transform::FromTransformMatrix(jointGlobalTransformMat * parentNodeGlobal.GetInverse());
	//				auto localframe1 = Transform::FromTransformMatrix(jointGlobalTransformMat * boneGlobalTransformMat.GetInverse());
	//				auto d6Joint = mheap::New<D6Joint>(parentObj->GetComponent<RigidBodyDynamic>(), localframe0, dynamic, localframe1);
	//				d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Y, D6Joint::MOTION_TYPE::FREE);
	//				d6Joint->SetMotion(D6Joint::MOTION_AXIS::SWING_Z, D6Joint::MOTION_TYPE::FREE);
	//			}
	//		}
	//	}
	//}

	newObj->Name() = "Skeleton";

	auto currentSceneEditorTab = dynamic_cast<SceneEditorTab*>(EditorContext::Get()->GetCurrentTab());
	if (currentSceneEditorTab)
	{
		currentSceneEditorTab->IndexObject(newObj);
	}

	if (m_animator->GetGameObject()->Parent().Get())
	{
		m_animator->GetGameObject()->Parent()->AddChild(newObj);
	}
	else
	{
		m_animator->GetGameObject()->GetScene()->AddObject(newObj);
	}
}

void AnimatorInspector::DrawDebugSkeleton()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics)
	{
		return;
	}



	//auto& model = m_animator->m_model3D;
	//auto& nodes = model->m_nodes;
	//auto& offsets = model->m_boneOffsetMatrixs;

	//auto& objGlobalTransform = m_animator->GetGameObject()->GetCommittedGlobalTransform();

	//std::vector<Mat4> nodeGlobals;
	//nodeGlobals.resize(nodes.size());

	//for (size_t i = 0; i < nodes.size(); i++)
	//{
	//	auto& node = nodes[i];
	//	if (node.boneId != INVALID_ID)
	//	{
	//		nodeGlobals[i] = offsets[node.boneId].GetInverse() * objGlobalTransform;
	//	}
	//}

	//for (size_t i = 0; i < nodes.size(); i++)
	//{
	//	auto& node = nodes[i];
	//	auto& global = nodeGlobals[i];
	//	if (node.boneId != INVALID_ID)
	//	{
	//		debugGraphics->DrawSphere(Sphere(global.Position(), 0.01f), { 1,0,0,1 });
	//		/*auto& mat = global;
	//		debugGraphics->DrawRay(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
	//		debugGraphics->DrawRay(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
	//		debugGraphics->DrawRay(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });*/
	//	}
	//}
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