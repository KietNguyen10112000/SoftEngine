#include "AnimatorEditorTab.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"

#include "DataInspector.h"
#include "AnimatorEditorSaveData.h"

#include "imgui/imgui.h"

#include "imgui-node-editor/imgui_node_editor.h"
#include "NodeEditorUtils/builders.h"
#include "NodeEditorUtils/widgets.h"

#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"

#include "Resources/Utils/Utils.h"

#include "FileChooser.h"

namespace ed = ax::NodeEditor;

struct AnimatorEditorTabExternNodeData
{
	struct AnimPlayerLayerExternData : public AnimatorEditorTab::NodeExternData
	{
		ID currentAnimId = INVALID_ID;

		virtual void OnNodesUpdated() override
		{
			auto layer = (AnimPlayerLayer*)node->layer;

			currentAnimId = INVALID_ID;
			for (auto& anim : tab->m_animator->m_model3D->m_animations)
			{
				if (anim == layer->m_animation)
				{
					currentAnimId = &anim - tab->m_animator->m_model3D->m_animations.data();
					break;
				}
			}

			assert(currentAnimId != INVALID_ID);
		}
	};

	struct AnimBlendLayerExternData : public AnimatorEditorTab::NodeExternData
	{
		Animation* animation = nullptr;
		ID currentAnimId = 0;

		float start = -1;
		float end = -1; 
		float fadeTime = 0;

		virtual void OnNodesUpdated() override
		{
			currentAnimId = 0;
			animation = tab->m_animator->m_model3D->m_animations[currentAnimId];
		}
	};
};

AnimatorEditorTab::AnimatorEditorTab(const String& modelPath, Scene* scene)
{
	m_modelPath = modelPath;

	m_nodeHeaderTexture = resource::Load<Texture2D>("Editor/BlueprintBackground.png");

	auto savePath = (EditorContext::GetInstance()->GetSavePath() + "AnimatorEditor/" + m_name + ".config.json");
	ed::Config config;
	config.SettingsFile = savePath.c_str();
	config.UserPointer = this;
	m_nodeEditorCtx = ed::CreateEditor(&config);
}

void AnimatorEditorTab::OnObjectsAdded(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnObjectsRemoved(std::vector<GameObject*>& objects)
{
}

void AnimatorEditorTab::OnRenderGUI()
{
	const float HEADER_HEIGHT = 65;

	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;

	{
		wflags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowPos({ center.x,HEADER_HEIGHT }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y - HEADER_HEIGHT));

		if (ImGui::Begin("Editor", 0, wflags))
		{
			//ImGui::Image(m_nodeHeaderTexture->GetNativeHandle(), { 100,100 });

			RenderBluePrintPanel();
			ImGui::End();
		}
	}
	
	{
		wflags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowPos({ 0,HEADER_HEIGHT }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 4.0f, viewPortSize.y / 2.0f));
		ImGui::Begin("Inspector ##AnimatorEditorTab", 0, wflags);

		if (ImGui::Button("Import Motion"))
		{
			auto path = FileChooser::OpenFileChooser("", false);

			std::vector<Resource<AnimMotion>> motions;
			if (ResourceUtils::LoadAnimMotion(path, motions) == 0)
			{
				for (auto& m : motions)
				{
					if (m_animator->m_model3D->FindAnimation(m) == nullptr)
					{
						auto animation = m_animator->m_model3D->AddAnimation(m);

						auto& state = m_animationsEditingState.emplace_back();
						state.name = animation->Name();
					}
				}
			}
			else
			{
				std::cerr << "Import Motion ERROR!\n";
			}
			
		}
		ImGui::Separator();

		m_objMetadata->ForEachProperties(
			[&](ClassMetadata* metadata, const char* propertyName, Accessor& accessor, size_t depth)
			{
				auto var = accessor.Get();
				if (var.Type() == VARIANT_TYPE::TRANSFORM3D)
				{
					DataInspector::Inspect(metadata, accessor, propertyName);
					return false;
				}

				return true;
			}, nullptr
		);

		ImGui::Separator();
		wflags = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
		//ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x, viewPortSize.y / 4.0f));
		ImGui::BeginChild("AnimMotions", {0,0}, true, wflags);
		/*if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_::ImGuiTableFlags_BordersInnerV))
		{
			for (auto& a : m_animator->m_model3D->m_animations)
			{
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(a->Name().c_str());

				ImGui::TableNextColumn();
				ImGui::TextUnformatted((a->GetMotion()->GetModelFilePath()).c_str());
			}
			ImGui::EndTable();
		}*/

		ImGui::TextUnformatted("Animations: ");
		if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH))
		{
			//ImGui::TableSetupColumn(nullptr);
			//ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 1000);
			//ImGui::TableHeadersRow();

			auto& animations = m_animator->m_model3D->m_animations;
			if (m_animationsEditingState.size() == 0)
			{
				m_animationsEditingState.resize(animations.size());
				for (size_t i = 0; i < animations.size(); i++)
				{
					auto& state = m_animationsEditingState[i];
					state.name = animations[i]->Name();
				}
			}

			size_t i = 0;
			for (auto& animation : animations)
			{
				auto& state = m_animationsEditingState[i];

				ImGui::TableNextColumn();
				ImGui::Text("[%d] ", i);

				ImGui::TableNextColumn();
				if (state.isEditingName)
				{
					ImGui::SetNextItemWidth(500);
					if (ImGui::InputText("##Name", m_inputName, IM_ARRAYSIZE(m_inputName), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						state.isEditingName = false;
						if (m_inputName[0])
						{
							state.name = m_inputName;
						}
					}
				}
				else
				{
					ImGui::Selectable(state.name.c_str());
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
					{
						for (auto& s : m_animationsEditingState)
						{
							s.isEditingName = false;
						}

						state.isEditingName = true;
						m_inputName[0] = 0;
					}
				}

				i++;
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		
		ImGui::End();
	}
}

void AnimatorEditorTab::OnRenderInGameDebugGraphics()
{
	EditorContext::OxyzRenderConfig config;
	config.RenderOxzGrid = true;
	config.AxisYLength = 200.0f;
	EditorContext::GetInstance()->RenderOxyz(config);
}

void AnimatorEditorTab::OnShow()
{
	m_onSaveListenerId = EditorContext::Get()->EventDispatcher()->AddListener(EditorContext::EVENT::MENU_ON_SAVE,
		[](EditorContext* ctx, int argc, void** argv, ID id)
		{
			auto path = *(String*)argv[0];
			auto tab = (AnimatorEditorTab*)id;

			AnimatorEditorSaveData data(tab);
			data.m_name = tab->m_name;
			data.m_objectUUID = tab->m_object->GetUUID();
			data.m_sceneUUID = tab->m_scene->GetUUID();
			data.m_cameraUUID = tab->m_cam->GetUUID();

			Serializer serializer = {};
			serializer.Serialize(tab->m_scene);
			serializer.Serialize(&data);
			serializer.SetRootUUID(data.GetUUID());

			if (path.empty())
			{
				path = tab->m_modelPath;
			}

			auto ext = FileUtils::GetExtension(path);
			if (ext != "json")
			{
				path = EditorContext::Get()->GetSavePath() + "AnimatorEditor/" + tab->m_name + ".json";
			}

			serializer.WriteToFile(path);
		}, 
		ID(this)
	);
}

void AnimatorEditorTab::OnHide()
{
	if (m_onSaveListenerId != INVALID_ID)
	{
		//auto d = EditorContext::Get()->EventDispatcher();
		EditorContext::Get()->EventDispatcher()->RemoveListener(m_onSaveListenerId);
		m_onSaveListenerId = INVALID_ID;
	}
}

void AnimatorEditorTab::OnOpen()
{
	Transform transform = {};

	if (!m_cam)
	{
		auto cameraObj = mheap::New<GameObject>();
		cameraObj->Name() = "#camera";
		auto fppCamScript = cameraObj->NewComponent<FPPCameraScript>();
		auto camera = cameraObj->NewComponent<Camera>();
		camera->Projection().SetPerspectiveFovLH(
			PI / 3.0f,
			Graphics::Get()->GetWindowWidth() / 2.0f / (float)Graphics::Get()->GetWindowHeight(),
			0.5f,
			1000.0f
		);
		fppCamScript->SetFPPScriptEnable(true);
		m_scene->AddObject(cameraObj);

		m_cam = cameraObj;
	}

	auto cam = m_cam->GetComponent<Camera>();
	m_cam->GetComponent<FPPCameraScript>()->SetFPPScriptEnable(true);
	m_scene->GetRenderingSystem()->HideCamera(cam);
	m_scene->GetRenderingSystem()->DisplayCamera(cam,
		GRAPHICS_VIEWPORT({ {0,0},{Graphics::Get()->GetWindowWidth() / 2,Graphics::Get()->GetWindowHeight()} })
	);

	if (m_object)
	{
		if (m_nodes.size() == 0)
		{
			BuildNodesFromAnimator();
		}
		return;
	}

	m_object = resource::Load<AnimModel>(m_modelPath)->MakeGameObject();
	m_animator = m_object->GetComponent<AnimatorSkeletalArray>();

	m_scene->AddObject(m_object);

	m_objMetadata = m_object->GetMetadata(0);

	if (m_nodes.size() == 0)
	{
		BuildNodesFromAnimator();
	}
}

void AnimatorEditorTab::OnClose()
{
	ed::DestroyEditor(m_nodeEditorCtx);
	m_nodeEditorCtx = nullptr;
}

void AnimatorEditorTab::WriteNodeDataToJson(Serializer* serializer, json& j) const
{
	ed::SetCurrentEditor(m_nodeEditorCtx);

	json nodesData;

	{
		json positions = json::array();
		for (size_t i = 0; i < m_nodes.size(); i++)
		{
			auto pos = ed::GetNodePosition(m_nodes[i]->nodeId);
			positions.push_back(Vec2(pos.x, pos.y));
		}

		nodesData["NodesPosition"] = positions;
	}

	{
		json states = json::array();
		for (size_t i = 0; i < m_animationsEditingState.size(); i++)
		{
			auto& state = m_animationsEditingState[i];

			json j1;
			j1["Name"] = state.name;

			states.push_back(j1);
		}

		nodesData["AnimationsEditingState"] = states;
	}

	j["EditorData"] = nodesData;
}

void AnimatorEditorTab::ReadNodeDataFromJson(Serializer* serializer, const json& j)
{
	if (!j.contains("EditorData"))
	{
		return;
	}

	auto& nodesData = j["EditorData"];

	if (nodesData.contains("NodesPosition"))
	{
		m_savedPositions.clear();

		auto& positions = nodesData["NodesPosition"];
		for (size_t i = 0; i < positions.size(); i++)
		{
			m_savedPositions.push_back(positions[i]);
		}
	}

	if (nodesData.contains("AnimationsEditingState"))
	{
		m_animationsEditingState.clear();

		auto& states = nodesData["AnimationsEditingState"];
		for (size_t i = 0; i < states.size(); i++)
		{
			auto& jstate = states[i];
			AnimationEditingState state = {};
			state.name = jstate["Name"];

			m_animationsEditingState.push_back(state);
		}
	}
}

void AnimatorEditorTab::BuildNodesFromAnimator()
{
	NodesBuilder builder;

	std::map<AnimLayer*, Node*>& animLayerToNode = builder.animLayerToNode;

	auto& layers = m_animator->m_animLayers;
	for (auto& layer : layers)
	{
		m_nodes.push_back(std::move(CreateNode(layer)));

		auto node = m_nodes.back().get();
		node->nodeIdx = m_nodes.size() - 1;
		animLayerToNode.insert({ layer,node });
	}

	ed::SetCurrentEditor(m_nodeEditorCtx);

	for (auto& node : m_nodes)
	{
		BuildNode(node.get(), builder);
	}

	for (auto& node : m_nodes)
	{
		if (node->externData)
		{
			node->externData->OnNodesUpdated();
		}
	}
}

AnimatorEditorTab::LAYER_TYPE::TYPE AnimatorEditorTab::GetNodeType(Node* node, void** concretePtr)
{
	auto type = node->layerType;
	switch (type)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		*concretePtr = dynamic_cast<AnimPlayerLayer*>(node->layer);
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
		*concretePtr = dynamic_cast<AnimBlendLayer*>(node->layer);
		break;
	default:
		assert(0);
		break;
	}

	return type;
}

AnimatorEditorTab::LAYER_TYPE::TYPE AnimatorEditorTab::GetLayerType(AnimLayer* layer)
{
	LAYER_TYPE::TYPE type = LAYER_TYPE::NONE;

	if (dynamic_cast<AnimPlayerLayer*>(layer))
	{
		type = LAYER_TYPE::ANIMATON_PLAYER;
	}
	else if (dynamic_cast<AnimBlendLayer*>(layer))
	{
		type = LAYER_TYPE::BLENDING;
	}
	else
	{
		assert(0);
	}

	return type;
}

UniquePtr<AnimatorEditorTab::Node> AnimatorEditorTab::CreateNode(AnimLayer* layer)
{
	UniquePtr<Node> node = std::make_unique<Node>();
	
	auto layerType = GetLayerType(layer);

	node->layer = layer;
	node->layerType = layerType;
	node->nodeId = GetNextId();
	node->outputPinId = GetNextId();

	switch (layerType)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		node->externData = new AnimatorEditorTabExternNodeData::AnimPlayerLayerExternData();
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
		node->externData = new AnimatorEditorTabExternNodeData::AnimBlendLayerExternData();
		break;
	default:
		assert(0);
		break;
	}

	if (node->externData)
	{
		node->externData->node = node.get();
		node->externData->tab = this;
	}

	return std::move(node);
}

std::vector<AnimLayer*> AnimatorEditorTab::GetInputLayers(AnimLayer* layer)
{
	std::vector<AnimLayer*> ret;

	auto type = GetLayerType(layer);
	switch (type)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
	{
		auto blendLayer = dynamic_cast<AnimBlendLayer*>(layer);
		{
			ret.push_back(blendLayer->m_input[0]);
			ret.push_back(blendLayer->m_input[1]);
		}
		break;
	}
	default:
		assert(0);
		break;
	}

	return ret;
}

void AnimatorEditorTab::BuildNode(Node* node, NodesBuilder& builder)
{
	auto type = node->layerType;

	auto inputs = GetInputLayers(node->layer);

	node->inputs.resize(inputs.size());
	for (auto& input : node->inputs)
	{
		input.linkId = INVALID_ID;
		input.pinId = GetNextId();
		input.node = nullptr;
	}

	for (size_t i = 0; i < inputs.size(); i++)
	{
		auto& layer = inputs[i];
		auto& input = node->inputs[i];

		assert(builder.animLayerToNode.find(layer) != builder.animLayerToNode.end());

		input.node = builder.animLayerToNode[layer];
		
		CreateLink(input.node, node, i);
	}

	if (m_savedPositions.size() > node->nodeIdx)
	{
		auto& pos = m_savedPositions[node->nodeIdx];
		ed::SetNodePosition(node->nodeId, { pos.x, pos.y });
	}
	else
	{
		ed::SetNodePosition(node->nodeId, { node->nodeIdx * 300.0f, (node->nodeIdx % 2) * 200.0f });
	}
}

void AnimatorEditorTab::RenderNodeHeader(void* p, Node* node, const char* title)
{
	namespace util = ax::NodeEditor::Utilities;

	util::BlueprintNodeBuilder& builder = *(util::BlueprintNodeBuilder*)p;

	ImColor headerColor = ImColor(128,195,255);
	if (!node->layer->IsEnable())
	{
		headerColor = ImColor(255,255,255);
	}
	builder.Header(headerColor);
	ImGui::TextUnformatted(title);
	ImGui::Dummy(ImVec2(0, 5));
	builder.EndHeader();
}

void AnimatorEditorTab::RenderNode_ANIMATON_PLAYER(Node* node, void* concretePtr)
{
	namespace util = ax::NodeEditor::Utilities;

	auto layer = (AnimPlayerLayer*)concretePtr;
	auto externData = (AnimatorEditorTabExternNodeData::AnimPlayerLayerExternData*)node->externData;
	auto& animations = m_animator->m_model3D->m_animations;

	util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
	builder.Begin(node->nodeId);
	{
		//ed::SetNodePosition(uniqueId, { 0,0 });

		RenderNodeHeader(&builder, node, "AnimPlayerLayer");

		ImGui::Dummy({ 218, 0 }); ImGui::SameLine();
		{
			assert(node->inputs.size() == 0);
			builder.Output(node->outputPinId);
			ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
			builder.EndOutput();
		}

		//auto curAnimation = layer->m_animation;
		ImGui::SetNextItemWidth(250);
		if (ed::BeginNodeCombo("##ChooseAnimation", m_animationsEditingState[externData->currentAnimId].name.c_str(), 0))
		{
			for (size_t i = 0; i < animations.size(); i++)
			{
				auto& animation = animations[i];
				auto& state = m_animationsEditingState[i];
				if (ImGui::Selectable(state.name.c_str()))
				{
					externData->currentAnimId = i;
					layer->SetAnimation(animation, -1, -1);
				}
			}
			ed::EndNodeCombo();
		}

	}
	builder.End();
}

void AnimatorEditorTab::RenderNode_BLENDING(Node* node, void* concretePtr)
{
	namespace util = ax::NodeEditor::Utilities;

	auto drawList = ed::GetNodeBackgroundDrawList(node->nodeId);

	auto layer = (AnimBlendLayer*)concretePtr;
	auto externData = (AnimatorEditorTabExternNodeData::AnimBlendLayerExternData*)node->externData;
	auto& animations = m_animator->m_model3D->m_animations;

	util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
	builder.Begin(node->nodeId);
	{
		//ed::SetNodePosition(uniqueId, { 0,0 });

		RenderNodeHeader(&builder, node, "AnimBlendLayer");

		{
			assert(node->inputs.size() == 2);

			ImGui::BeginGroup();

			{
				auto& input = node->inputs[0];
				builder.Input(input.pinId);
				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
				ImGui::TextUnformatted("Layer 0");
				builder.EndOutput();
			}

			{
				auto& input = node->inputs[1];
				builder.Input(input.pinId);
				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
				ImGui::TextUnformatted("Layer 1");
				builder.EndOutput();
			}

			ImGui::EndGroup();
		}

		ImGui::SameLine(); ImGui::Dummy({ 20, 0 }); ImGui::SameLine();

		{
			builder.Output(node->outputPinId);
			ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
			builder.EndOutput();
		}

		{
			builder.Separator();

			ImGui::SetNextItemWidth(100);
			if (ImGui::ArrowButton("Fade", ImGuiDir_::ImGuiDir_Right))
			{
				layer->FadeTo(externData->animation, externData->start, externData->end, externData->fadeTime);
			}
			ImGui::SameLine(); //ImGui::Dummy({ 20, 0 }); ImGui::SameLine();
			ImGui::TextUnformatted("Fade Animation");

			ImGui::SetNextItemWidth(250);
			if (ed::BeginNodeCombo("##ChooseAnimation", m_animationsEditingState[externData->currentAnimId].name.c_str(), 0))
			{
				for (size_t i = 0; i < animations.size(); i++)
				{
					auto& animation = animations[i];
					auto& state = m_animationsEditingState[i];
					if (ImGui::Selectable(state.name.c_str()))
					{
						externData->currentAnimId = i;
						externData->animation = animation;
					}
				}
				ed::EndNodeCombo();
			}

			auto anim = externData->animation;
			ImGui::SetNextItemWidth(100);
			ImGui::DragFloat("Start", &externData->start, 0.001f, -1.0f, INFINITY);
			//ImGui::SameLine();

			ImGui::SetNextItemWidth(100);
			ImGui::DragFloat("End", &externData->end, 0.001f, -1.0f, INFINITY);
			//ImGui::SameLine(); 

			ImGui::SetNextItemWidth(100);
			ImGui::DragFloat("Fade Time", &externData->fadeTime, 0.001f, 0.001f, INFINITY);
			//ImGui::SameLine();

			
		}
	}
	builder.End();
}

void AnimatorEditorTab::RenderNode(Node* node)
{
	void* ptr = nullptr;
	auto type = GetNodeType(node, &ptr);

	switch (type)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		RenderNode_ANIMATON_PLAYER(node, ptr);
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
		RenderNode_BLENDING(node, ptr);
		break;
	default:
		assert(0);
		break;
	}
}

void AnimatorEditorTab::RenderBluePrintPanel()
{
	namespace util = ax::NodeEditor::Utilities;

	ed::SetCurrentEditor(m_nodeEditorCtx);
	ed::Begin("Node Editor", ImVec2(0.0, 0.0f));

	int uniqueId = 1;

	//{
	//	util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
	//	builder.Begin(++uniqueId);
	//	{
	//		//ed::SetNodePosition(uniqueId, { 0,0 });

	//		builder.Header();
	//		ImGui::TextUnformatted("Node A");
	//		ImGui::Dummy(ImVec2(0, 5));
	//		builder.EndHeader();

	//		{
	//			{
	//				builder.Input(++uniqueId);
	//				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false);
	//				ImGui::SameLine();
	//				ImGui::TextUnformatted("Input");
	//				//ImGui::SameLine();
	//				//ImGui::Button("Hello");
	//				builder.EndInput();
	//			}

	//			ImGui::SameLine(0, 30);
	//			{
	//				builder.Output(++uniqueId);
	//				ImGui::TextUnformatted("Output"); ImGui::SameLine();
	//				ImGui::Button("Hello"); ImGui::SameLine();
	//				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false);
	//				builder.EndOutput();
	//			}
	//		}

	//		ImGui::Dummy(ImVec2(200, 100));
	//	}
	//	builder.End();
	//}
	//
	//{
	//	util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
	//	builder.Begin(++uniqueId);
	//	{
	//		//ed::SetNodePosition(uniqueId, { 0,0 });

	//		builder.Header();
	//		ImGui::TextUnformatted("Node B");
	//		ImGui::Dummy(ImVec2(0, 5));
	//		builder.EndHeader();

	//		{
	//			{
	//				builder.Input(++uniqueId);
	//				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false);
	//				ImGui::SameLine();
	//				ImGui::TextUnformatted("Input");
	//				//ImGui::SameLine();
	//				//ImGui::Button("Hello");
	//				builder.EndInput();
	//			}

	//			ImGui::SameLine(0, 30);
	//			{
	//				builder.Output(++uniqueId);
	//				ImGui::TextUnformatted("Output"); ImGui::SameLine();
	//				ImGui::Button("Hello"); ImGui::SameLine();
	//				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false);
	//				builder.EndOutput();
	//			}
	//		}

	//		ImGui::Dummy(ImVec2(200, 100));
	//	}
	//	builder.End();
	//}

	for (auto& node : m_nodes)
	{
		RenderNode(node.get());
	}

	if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
	{
		ed::PinId inputPinId, outputPinId;
		if (ed::QueryNewLink(&inputPinId, &outputPinId))
		{
			if (inputPinId && outputPinId)
			{
				if (ed::AcceptNewItem())
				{
					auto src = GetNode(ID(inputPinId));
					auto dest = GetNode(ID(outputPinId));

					if (src->outputPinId == ID(inputPinId))
					{
						std::swap(src, dest);
						std::swap(inputPinId, outputPinId);
					}

					ID destIdx = INVALID_ID;
					for (size_t i = 0; i < dest->inputs.size(); i++)
					{
						if (dest->inputs[i].pinId == ID(outputPinId))
						{
							destIdx = i;
						}
					}
					assert(destIdx != INVALID_ID);

					if (dest->inputs[destIdx].linkId == INVALID_ID)
					{
						CreateLink(src, dest, destIdx);
					}
				}
			}
		}

	}

	ed::EndCreate();

	for (auto& link : m_links)
	{
		ed::Link(link.linkId, link.src->outputPinId, link.dest->inputs[link.destIdx].pinId, { 1,1,1,1 }, 2);
		//ed::Flow(link.linkId);
	}

	ed::End();

	if (m_isFirstRender < 5)
	{
		ed::NavigateToContent();
		m_isFirstRender++;
	}
	ed::SetCurrentEditor(nullptr);
}
