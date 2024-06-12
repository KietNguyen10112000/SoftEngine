#include "AnimatorEditorTab.h"

#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"
#include "Resources/Utils/Utils.h"

#include "DataInspector.h"
#include "AnimatorEditorSaveData.h"

#include "imgui/imgui.h"

#include "imgui-node-editor/imgui_node_editor.h"
#include "NodeEditorUtils/builders.h"
#include "NodeEditorUtils/widgets.h"

#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimMixLayer.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Graphics/DebugGraphics.h"

#include "FileChooser.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "ImGuiExtern.h"

#include <queue>

namespace ed = ax::NodeEditor;

struct AnimPlayerLayerNode : public AnimatorEditorTab::Node
{
	ID currentAnimId = INVALID_ID;

	AnimPlayerLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	virtual void OnBuiltDone() override
	{
		auto layer = (AnimPlayerLayer*)this->layer;

		currentAnimId = INVALID_ID;
		for (auto& anim : tab->m_animator->m_model3D->m_animations)
		{
			if (anim == layer->m_animation)
			{
				currentAnimId = &anim - tab->m_animator->m_model3D->m_animations.data();
				break;
			}
		}

		//assert(currentAnimId != INVALID_ID);
		if (currentAnimId == INVALID_ID)
		{
			layer->SetAnimation(tab->m_animator->m_model3D->m_animations[0], -1, -1);
			currentAnimId = 0;
		}
	}

	virtual std::vector<AnimLayer*> GetInputLayers() override
	{
		return {};
	}

	virtual void ProcessSetInputLayers() override
	{

	}

	virtual void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimPlayerLayer*)this->layer;
		auto& animations = tab->m_animator->m_model3D->m_animations;

		if (currentAnimId == INVALID_ID)
		{
			OnBuiltDone();
		}

		//util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
		//builder.Begin(node->nodeId);
		{
			//ed::SetNodePosition(uniqueId, { 0,0 });

			tab->RenderNodeHeader(&builder, this, "AnimPlayerLayer", 345);

			ImGui::Dummy({ 318, 0 }); ImGui::SameLine();
			{
				assert(inputs.size() == 0);
				builder.Output(outputPinId);
				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
				builder.EndOutput();
			}

			//auto curAnimation = layer->m_animation;
			builder.Separator();

			ImGui::SetNextItemWidth(350);
			if (ed::BeginNodeCombo("##ChooseAnimation", tab->m_animationsEditingState[currentAnimId].name.c_str(), 0))
			{
				for (size_t i = 0; i < animations.size(); i++)
				{
					auto& animation = animations[i];
					auto& state = tab->m_animationsEditingState[i];
					if (ImGui::Selectable(state.name.c_str()))
					{
						currentAnimId = i;
						layer->SetAnimation(animation, -1, -1);
					}
				}
				ed::EndNodeCombo();
			}

			builder.Separator();
			if (layer->m_animation != nullptr)
			{
				float start = layer->m_startTick / layer->m_ticksPerSecond;
				float end = (layer->m_startTick + layer->m_tickDuration) / layer->m_ticksPerSecond;
				float t = (layer->m_t + layer->m_startTick) / layer->m_ticksPerSecond;
				ImGui::SetNextItemWidth(250);
				if (ImGui::SliderFloat("Track", &t, start, end))
				{
					layer->SetCurrentTime(t);
				}

				auto curEnd = end;
				end = (layer->m_animation->GetTickDuration()) / layer->m_animation->GetTicksPerSecond();
				start = layer->m_startTick / layer->m_animation->GetTicksPerSecond();
				ImGui::SetNextItemWidth(250);
				if (ImGui::SliderFloat("Start", &start, 0, end))
				{
					layer->SetStartTime(start);
				}

				ImGui::SetNextItemWidth(250);
				if (ImGui::SliderFloat("End", &curEnd, 0, end))
				{
					layer->SetEndTime(curEnd);
				}

				auto curDur = layer->m_tickDuration / layer->m_ticksPerSecond;
				ImGui::SetNextItemWidth(250);
				if (ImGui::DragFloat("Duration", &curDur, 0.001f, 0.001f, INFINITY))
				{
					layer->SetDuration(curDur);
				}
			}

		}
		//builder.End();
	}

	virtual int ValidateNewInput(Node* input, ID inputIdx, String& errDesc) override
	{
		return 0;
	}

	virtual int ValidateBeforeBuilt(String& errDesc) override
	{
		return 0;
	}

	virtual void WriteToJson(json& json) const override
	{

	}

	virtual void ReadFromJson(const json& json) override
	{

	}
};

struct AnimBlendLayerNode : public AnimatorEditorTab::Node
{
	Animation* animation = nullptr;
	ID currentAnimId = 0;

	float start = -1;
	float end = -1;
	float fadeTime = 0;

	AnimBlendLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	virtual void OnBuiltDone() override
	{
		currentAnimId = 0;
		animation = tab->m_animator->m_model3D->m_animations[currentAnimId];
	}

	virtual std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimBlendLayer*)this->layer;
		return {
			layer->m_input[0],
			layer->m_input[1],
		};
	}

	virtual void ProcessSetInputLayers() override
	{
		auto layer = (AnimBlendLayer*)this->layer;
		layer->m_input[0] = GetInputLayerFromNode(0);
		layer->m_input[1] = GetInputLayerFromNode(1);
	}

	virtual void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimBlendLayer*)this->layer;
		auto& animations = tab->m_animator->m_model3D->m_animations;

		//util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
		//builder.Begin(node->nodeId);
		{
			//ed::SetNodePosition(uniqueId, { 0,0 });

			tab->RenderNodeHeader(&builder, this, "AnimBlendLayer", 250);

			{
				//assert(committedInputs.size() == 2);

				ImGui::BeginGroup();

				{
					auto& input = inputs[0];
					builder.Input(input.pinId);
					ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
					ImGui::TextUnformatted("Layer 0");
					builder.EndOutput();
				}

				{
					auto& input = inputs[1];
					builder.Input(input.pinId);
					ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
					ImGui::TextUnformatted("Layer 1");
					builder.EndOutput();
				}

				ImGui::EndGroup();
			}

			ImGui::SameLine(); ImGui::Dummy({ 120, 0 }); ImGui::SameLine();

			{
				builder.Output(outputPinId);
				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
				builder.EndOutput();
			}

			{
				builder.Separator();

				ImGui::SetNextItemWidth(100);
				if (ImGui::ArrowButton("Fade", ImGuiDir_::ImGuiDir_Right))
				{
					layer->FadeTo(animation, start, end, fadeTime);

					auto UpdateInput = [](Node* input)
					{
						auto playerLayerNode= dynamic_cast<AnimPlayerLayerNode*>(input);
						if (playerLayerNode)
						{
							playerLayerNode->currentAnimId = INVALID_ID;
						}
					};

					if (committedInputs[0].link)
						UpdateInput(committedInputs[0].link->src);

					if (committedInputs[1].link)
						UpdateInput(committedInputs[1].link->src);
				}
				ImGui::SameLine(); //ImGui::Dummy({ 20, 0 }); ImGui::SameLine();
				ImGui::TextUnformatted("Fade Animation");

				ImGui::SetNextItemWidth(250);
				if (ed::BeginNodeCombo("##ChooseAnimation", tab->m_animationsEditingState[currentAnimId].name.c_str(), 0))
				{
					for (size_t i = 0; i < animations.size(); i++)
					{
						auto& animation = animations[i];
						auto& state = tab->m_animationsEditingState[i];
						if (ImGui::Selectable(state.name.c_str()))
						{
							currentAnimId = i;
							this->animation = animation;
						}
					}
					ed::EndNodeCombo();
				}

				auto anim = animation;
				ImGui::SetNextItemWidth(100);
				ImGui::DragFloat("Start", &start, 0.001f, -1.0f, INFINITY);
				//ImGui::SameLine();

				ImGui::SetNextItemWidth(100);
				ImGui::DragFloat("End", &end, 0.001f, -1.0f, INFINITY);
				//ImGui::SameLine(); 

				ImGui::SetNextItemWidth(100);
				ImGui::DragFloat("Fade Time", &fadeTime, 0.001f, 0.001f, INFINITY);
				//ImGui::SameLine();


			}
		}
		//builder.End();
	}

	virtual int ValidateNewInput(Node* input, ID inputIdx, String& errDesc) override
	{
		return 0;
	}

	virtual int ValidateBeforeBuilt(String& errDesc) override
	{
		return 0;
	}

	virtual void WriteToJson(json& json) const override
	{

	}

	virtual void ReadFromJson(const json& json) override
	{

	}
};

struct AnimMixLayerNode : public AnimatorEditorTab::Node
{
	std::vector<std::vector<float>> layerWeights;
	ID selectedInputId = INVALID_ID;

	float editingWeight = 0.0f;

	AnimMixLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	// Inherited via Node
	void OnBuiltDone() override
	{
		auto layer = (AnimMixLayer*)this->layer;
		layerWeights.resize(layer->m_inputs.size());
		size_t i = 0;
		for (auto& input : layer->m_inputs)
		{
			layerWeights[i] = input.weight;
			i++;
		}
	}

	std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimMixLayer*)this->layer;
		std::vector<AnimLayer*> ret; 
		for (auto& input : layer->m_inputs)
		{
			ret.push_back(input.layer);
		}
		return ret;
	}

	void ProcessSetInputLayers() override
	{
		auto layer = (AnimMixLayer*)this->layer;
		auto& inputs = layer->m_inputs;
		inputs.resize(GetInputLayerFromNodeCount());

		for (size_t i = 0; i < inputs.size(); i++)
		{
			auto& input = inputs[i];
			input.layer = GetInputLayerFromNode(i);
			input.weight = layerWeights[i];
		}
	}

	void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimMixLayer*)this->layer;
		auto& animations = tab->m_animator->m_model3D->m_animations;

		tab->RenderNodeHeader(&builder, this, "AnimMixLayer", 250);

		{
			ImGui::BeginGroup();

			size_t i = 0;
			for (auto& input : inputs)
			{
				builder.Input(input.pinId);
				ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
				ImGui::TextUnformatted(String::Format("Layer {}", i).c_str());
				builder.EndOutput();
				i++;
			}

			if (i == 0)
			{
				ImGui::Dummy({ 90, 0 });
			}

			ImGui::EndGroup();
		}

		ImGui::SameLine(); ImGui::Dummy({ 120, 0 }); ImGui::SameLine();

		{
			builder.Output(outputPinId);
			ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
			builder.EndOutput();
		}

		builder.Separator();

		{
			ImGui::Dummy({ 120, 25 });
		}
	}

	virtual bool RenderCustomInspector() override
	{
		if (!tab->m_isEnableTPoseMode)
		{
			return false;
		}

		if (ImGui::Button("Add Input"))
		{
			EmplaceBackInput();
		}

		ImGui::SameLine();

		if (ImGui::Button("Delete Selected Input"))
		{
			DeleteSelectedInput();
		}

		//auto wflags = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
		//ImGui::BeginChild("ChooseInput", { 0,ImGui::GetWindowSize().y / 4.0f - 50.0f }, true, wflags);
		String previewLabel = selectedInputId == INVALID_ID ? "<Add input to select>" : String::Format("Input {}", selectedInputId);
		if (ImGui::BeginCombo("Choose Input", previewLabel.c_str()))
		{
			size_t i = 0;
			for (auto& input : inputs)
			{
				if (ImGui::Selectable(String::Format("Input {}", i).c_str(), i == selectedInputId))
				{
					SetSelectedInput(i);
				}
				i++;
			}

			ImGui::EndCombo();
		}

		if (selectedInputId != INVALID_ID)
		{
			ImGui::Separator();
			ImGui::DragFloat("Weight", &editingWeight, 0.001f, 0, INFINITY); 
			if (ImGui::Button("Set Weight To Selected Bone(s)"))
			{
				auto& weights = layerWeights[selectedInputId];
				size_t i = 0;
				for (auto& node : tab->m_modelNodes)
				{
					if (node->isSelected)
					{
						weights[i] = editingWeight;
					}
					i++;
				}
			}
			ImGui::Separator();
		}
		
		//ImGui::EndChild();

		return true;
	};

	virtual void RenderCustomModelTreeNode(AnimatorEditorTab::ModelNode* node)
	{
		if (selectedInputId == INVALID_ID)
		{
			return;
		}

		ImGui::PushID(node->nodeIdx);
		ImGui::SetNextItemWidth(60);
		ImGui::DragFloat("", &layerWeights[selectedInputId][node->nodeIdx], 0.001f, 0, INFINITY);
		ImGui::PopID();
	};

	int ValidateNewInput(Node* input, ID inputIdx, String& errDesc) override
	{
		return 0;
	}

	int ValidateBeforeBuilt(String& errDesc) override
	{
		return 0;
	}

	void WriteToJson(json& json) const override
	{
	}

	void ReadFromJson(const json& json) override
	{
	}

	void EmplaceBackInput()
	{
		auto layer = (AnimMixLayer*)this->layer;

		auto& weight = layerWeights.emplace_back();
		weight.resize(layer->NodeGlobalTransforms().size());

		ResizeInputs(inputs.size() + 1);
	}

	void SetSelectedInput(ID id)
	{
		selectedInputId = id;
	}

	void DeleteSelectedInput()
	{
		if (selectedInputId == INVALID_ID)
		{
			return;
		}

		inputs.erase(inputs.begin() + selectedInputId);
		layerWeights.erase(layerWeights.begin() + selectedInputId);

		if (inputs.size() == 0)
		{
			selectedInputId = INVALID_ID;
		}
		else
		{
			if (selectedInputId != 0)
			{
				selectedInputId = selectedInputId - 1;
			}
		}

		SetSelectedInput(selectedInputId);
	}
};

struct AnimatorEditorTPoseLayer : public AnimLayer
{
public:
	SERIALIZABLE_CLASS(AnimatorEditorTPoseLayer, SERIALIZABLE_MEM_RAW);

	bool m_once = true;
	float m_coeff = 0.0f;

	// Inherited via AnimLayer
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
	{
	}

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
	{
	}

	Handle<ClassMetadata> GetMetadata(size_t sign) override
	{
		return Handle<ClassMetadata>();
	}

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
	{
	}

	void Run(float dt) override
	{
		if (!m_once)
		{
			return;
		}

		m_once = false;

		auto& model = m_model;
		auto& nodes = model->m_nodes;

		auto& offsetMatrix = model->m_boneOffsetMatrixs;
		for (size_t i = 0; i < m_globalTransforms.size(); i++)
		{
			auto& node = nodes[i];
			if (node.boneId != INVALID_ID)
			{
				m_globalTransforms[i] = offsetMatrix[node.boneId].GetInverse();
				m_globalTransforms[i] *= m_coeff;
			}
		}

		for (auto& aabb : m_meshesAABB) 
		{
			aabb = AABox({ 0,0,0 }, { 1000,1000,1000 });
		}
	}

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
	//ImGui::ShowDemoWindow();

	if (m_tposeMode != 0)
	{
		m_tposeLayer->Run(0);
		m_animator->UpdateDataToRenderer(m_scene, m_tposeLayer);
		m_tposeMode--;
	}

	if (m_isEnableTPoseMode && !m_isEnableModelInTPoseMode)
	{
		RenderModelSkeleton();
	}

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

			bool enableTPose = m_isEnableTPoseMode;
			if (ImGui::ToggleButton("TPoseToggle", &enableTPose))
			{
				if (m_isEnableTPoseMode)
				{
					SetTPoseMode(false);
				}
				else
				{
					SetTPoseMode(true);
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("TPose Mode");

			ImGui::SameLine(0, 20);
			if (ImGui::Button(ICON_FA_HAMMER "  Build"))
			{
				BuildGraph();
				m_isBuilding = true;
			}

			if (m_isBuilding)
			{
				ImGui::SameLine();
				//ImGui::Spinner("Building", 12, 2, ImColor(255,255,255));

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, { 0,1,0,1 });
				char buf[32];
				sprintf(buf, "%d/%d", int(m_buildingNow), int(m_buildingTotal));
				ImGui::ProgressBar(float(m_buildingNow) / float(m_buildingTotal), ImVec2(0.f, 0.f), buf);
				ImGui::PopStyleColor();

				if (m_buildingNow == m_buildingTotal)
				{
					// recollect handle for further uses
					WaitForDoneBuilding();
				}
			}
			else if (m_lastBuildCode != 0)
			{
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, m_lastBuildCode == 1 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1));
				ImGui::TextUnformatted(m_lastBuildCode == 1 ? "Build successed!" : "Build failed!");
				ImGui::PopStyleColor();
			}

			bool disableEditor = m_isBuilding || m_isEnableTPoseMode;
			ImGui::BeginDisabled(disableEditor);
			RenderBluePrintPanel();
			ImGui::EndDisabled();
			ImGui::End();
		}
	}
	
	ed::SetCurrentEditor(m_nodeEditorCtx);
	std::vector<ed::NodeId> selectedNodes;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	auto numSelectedNodes = ed::GetSelectedNodes(selectedNodes.data(), selectedNodes.size());
	selectedNodes.resize(numSelectedNodes);
	Node* selectedNode = nullptr;
	if (selectedNodes.size() == 1)
	{
		auto& nodeId = selectedNodes[0];
		auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
			[&](const UniquePtr<Node>& node)
			{
				return node->nodeId == ID(nodeId);
			}
		);
		selectedNode = (*it).get();
	}

	{
		wflags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowPos({ 0,HEADER_HEIGHT }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 4.0f, viewPortSize.y - HEADER_HEIGHT));
		ImGui::Begin("Inspector ##AnimatorEditorTab", 0, wflags);

		if (m_isEnableTPoseMode)
		{
			if (ImGui::ToggleButton("RenderModelInTPoseModeToggle", &m_isEnableModelInTPoseMode))
			{
				if (m_tposeLayer)
				{
					auto layer = (AnimatorEditorTPoseLayer*)m_tposeLayer;
					layer->m_once = true;
					layer->m_coeff = m_isEnableModelInTPoseMode ? 1.0f : 0.0f;
					layer->Run(0);
					m_animator->UpdateDataToRenderer(m_scene, m_tposeLayer);
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Show Mesh");

			ImGui::SameLine();
			if (ImGui::Button("Expand All"))
			{
				m_root->ForEach(
					[](ModelNode* node)
					{
						node->isTryingExpand = true;
					}
				);
			}

			auto wflags = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
			ImGui::BeginChild("ModelHierarchy", { 0,ImGui::GetWindowSize().y / 2.0f - 50.0f }, true, wflags);
			RenderModelNodeHierarchy(
				[](ModelNode* node, void* ptr) 
				{
					if (ptr == nullptr)
					{
						return;
					}

					((Node*)ptr)->RenderCustomModelTreeNode(node);
				}, 
				selectedNode
			);
			ImGui::EndChild();

			ImGui::BeginChild("CustomInspector", { 0,0 }, true, wflags);
			selectedNode&& selectedNode->RenderCustomInspector();
			ImGui::EndChild();
		}
		else if (!selectedNode || !selectedNode->RenderCustomInspector())
		{
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
			ImGui::BeginChild("AnimMotions", { 0,viewPortSize.y / 4.0f }, true, wflags);
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
		}

		//// test
		//{
		//	wflags = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
		//	ImGui::BeginChild("Test", { 0,0 }, true, wflags);
		//	RenderModelNodeHierarchy();
		//	ImGui::EndChild();
		//}
		
		ImGui::End();
	}
}

void AnimatorEditorTab::OnRenderInGameDebugGraphics()
{
	EditorContext::OxyzRenderConfig config;
	config.RenderOxzGrid = true;
	config.AxisYLength = 200.0f;
	config.AxisYColor = { 1,1,1,0.5f };
	EditorContext::GetInstance()->RenderOxyz(config);
}

void AnimatorEditorTab::OnShow()
{
	m_onSaveListenerId = EditorContext::Get()->EventDispatcher()->AddListener(EditorContext::EVENT::MENU_ON_SAVE,
		[](EditorContext* ctx, int argc, void** argv, ID id)
		{
			auto path = *(String*)argv[0];
			auto tab = (AnimatorEditorTab*)id;

			if (tab->m_isBuilding)
			{
				std::cerr << "Can not save while building!!!\n";
				return;
			}

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

	m_tposeLayer = m_animator->NewAnimLayer<AnimatorEditorTPoseLayer, true>();

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
		else
		{
			OnBuildNodesDone();
		}

		BuildModelHierarchy();
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

	BuildModelHierarchy();
}

void AnimatorEditorTab::OnClose()
{
	if (m_isBuilding)
	{
		std::cerr << "Wait for done building task ... \n";
		m_isRequestClosing = true;
		WaitForDoneBuilding();
	}

	if (m_tposeLayer)
	{
		delete m_tposeLayer;
		m_tposeLayer = nullptr;
	}

	{
		for (auto& node : m_modelNodes)
		{
			delete node;
		}
		m_modelNodes.clear();
	}

	ed::DestroyEditor(m_nodeEditorCtx);
	m_nodeEditorCtx = nullptr;
}

bool AnimatorEditorTab::IsCloseable()
{
	return !m_isBuilding;
}

void AnimatorEditorTab::WriteNodeDataToJson(Serializer* serializer, json& j) const
{
	ed::SetCurrentEditor(m_nodeEditorCtx);

	json nodesData;

	{
		json nodes = json::array();
		for (size_t i = 0; i < m_nodes.size(); i++)
		{
			auto node = m_nodes[i].get();
			auto pos = ed::GetNodePosition(m_nodes[i]->nodeId);

			json jnode;

			jnode["LayerType"] = node->layerType;
			jnode["LayerIdx"] = node->layerIdx;
			jnode["NodeId"] = node->nodeId;
			jnode["OutputPinId"] = node->outputPinId;

			/*json inputs = json::array();
			for (auto& input : node->inputs)
			{
				json jinput;
				jinput["LinkId"] = input.linkId;
				jinput["PinId"] = input.pinId;
				jinput["NodeId"] = input.node->nodeId;
				inputs.push_back(jinput);
			}
			jnode["Inputs"] = inputs;*/

			jnode["Position"] = Vec2(pos.x, pos.y);

			node->WriteToJson(jnode);

			nodes.push_back(jnode);
		}

		nodesData["Nodes"] = nodes;

		json links = json::array();
		for (size_t i = 0; i < m_links.size(); i++)
		{
			auto& link = m_links[i];

			json jlink;
			jlink["SrcNodeId"] = link->src->nodeId;
			jlink["DestNodeId"] = link->dest->nodeId;
			jlink["DestNodeIdx"] = link->destIdx;

			links.push_back(jlink);
		}

		nodesData["Links"] = links;
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

	nodesData["NextId"] = m_nextId;
	j["EditorData"] = nodesData;
}

void AnimatorEditorTab::ReadNodeDataFromJson(Serializer* serializer, const json& j)
{
	ed::SetCurrentEditor(m_nodeEditorCtx);

	if (!j.contains("EditorData"))
	{
		return;
	}

	auto& nodesData = j["EditorData"];

	if (nodesData.contains("Nodes"))
	{
		std::map<ID, Node*> nodeIdToNode;

		m_nextId = nodesData["NextId"];

		json nodes = nodesData["Nodes"];
		for (size_t i = 0; i < nodes.size(); i++)
		{
			auto& jnode = nodes[i];

			ID layerIdx = jnode["LayerIdx"];
			LAYER_TYPE::TYPE layerType = jnode["LayerType"];
			AnimLayer* layer = layerIdx == INVALID_ID ? CreateLayer(layerType) : m_animator->m_animLayers[layerIdx];
			auto node = CreateNode(layer);

			node->layerType = layerType;
			node->layerIdx = layerIdx;
			node->nodeId = jnode["NodeId"];
			node->nodeIdx = i;
			node->outputPinId = jnode["OutputPinId"];

			/*json& inputs = jnode["Inputs"];
			for (size_t j = 0; j < inputs.size(); j++)
			{
				auto& jinput = inputs[j];
				Node::Input input;
				input.linkId = jinput["LinkId"];
				input.pinId = jinput["PinId"];
				node->inputs.push_back(input);
			}*/

			node->ReadFromJson(jnode);

			Vec2 pos = jnode["Position"];
			ed::SetNodePosition(node->nodeId, ImVec2(pos.x, pos.y));

			nodeIdToNode.insert({ node->nodeId,node.get() });

			m_nodes.push_back(std::move(node));
		}

		/*for (size_t i = 0; i < nodes.size(); i++)
		{
			auto node = m_nodes[i].get();
			auto& jnode = nodes[i];

			json& inputs = jnode["Inputs"];
			for (size_t j = 0; j < inputs.size(); j++)
			{
				auto& jinput = inputs[j];
				node->inputs[j].node = nodeIdToNode[ID(jinput["NodeId"])];
			}

			node->ReadFromJson(jnode);
		}*/

		auto& links = nodesData["Links"];
		for (size_t i = 0; i < links.size(); i++)
		{
			auto& jlink = links[i];
			ID srcNodeId = jlink["SrcNodeId"];
			ID destNodeId = jlink["DestNodeId"];
			ID destNodeIdx = jlink["DestNodeIdx"];

			CreateLink(nodeIdToNode[srcNodeId], nodeIdToNode[destNodeId], destNodeIdx);
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
		node->layerIdx = m_nodes.size() - 1;
		node->nodeIdx = node->layerIdx;
		animLayerToNode.insert({ layer,node });
	}

	ed::SetCurrentEditor(m_nodeEditorCtx);

	for (auto& node : m_nodes)
	{
		BuildNode(node.get(), builder);
	}

	OnBuildNodesDone();
}

void AnimatorEditorTab::BuildModelHierarchy()
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
}

void AnimatorEditorTab::OnBuildNodesDone()
{
	for (auto& node : m_nodes)
	{
		node->OnBuiltDone();
		node->Commit();
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
	case AnimatorEditorTab::LAYER_TYPE::MIXING:
		*concretePtr = dynamic_cast<AnimMixLayer*>(node->layer);
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
	else if (dynamic_cast<AnimMixLayer*>(layer))
	{
		type = LAYER_TYPE::MIXING;
	}
	else
	{
		assert(0);
	}

	return type;
}

AnimLayer* AnimatorEditorTab::CreateLayer(LAYER_TYPE::TYPE type)
{
	AnimLayer* layer = nullptr;

	switch (type)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		layer = m_animator->NewAnimLayer<AnimPlayerLayer, true>();
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
		layer = m_animator->NewAnimLayer<AnimBlendLayer, true>();
		break; 
	case AnimatorEditorTab::LAYER_TYPE::MIXING:
		layer = m_animator->NewAnimLayer<AnimMixLayer, true>();
		break;
	default:
		assert(0);
		break;
	}

	//m_animator->m_animLayers.pop_back();

	return layer;
}

UniquePtr<AnimatorEditorTab::Node> AnimatorEditorTab::CreateNode(AnimLayer* layer)
{
	auto layerType = GetLayerType(layer);
	UniquePtr<Node> node;

	switch (layerType)
	{
	case AnimatorEditorTab::LAYER_TYPE::ANIMATON_PLAYER:
		node = std::make_unique<AnimPlayerLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLENDING:
		node = std::make_unique<AnimBlendLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::MIXING:
		node = std::make_unique<AnimMixLayerNode>(this);
		break;
	default:
		assert(0);
		break;
	}

	node->layer = layer;
	node->layerType = layerType;
	node->nodeId = GetNextId();
	node->outputPinId = GetNextId();
	node->tab = this;
	node->ResizeInputs(node->GetInputLayers().size());

	return std::move(node);
}

void AnimatorEditorTab::BuildNode(Node* node, NodesBuilder& builder)
{
	auto type = node->layerType;

	auto inputs = node->GetInputLayers();

	for (size_t i = 0; i < inputs.size(); i++)
	{
		auto& layer = inputs[i];
		//auto& input = node->inputs[i];

		assert(builder.animLayerToNode.find(layer) != builder.animLayerToNode.end());

		auto src = builder.animLayerToNode[layer];
		
		CreateLink(src, node, i);
	}

	{
		ed::SetNodePosition(node->nodeId, { node->nodeIdx * 500.0f, (node->nodeIdx % 2) * 500.0f });
	}
}

void AnimatorEditorTab::RenderNodeHeader(void* p, Node* node, const char* title, float nodeWidth)
{
	namespace util = ax::NodeEditor::Utilities;

	util::BlueprintNodeBuilder& builder = *(util::BlueprintNodeBuilder*)p;

	ImColor headerColor = ImColor(128,195,255);
	if (!node->layer->IsEnable())
	{
		headerColor = ImColor(255,255,255);
	}
	builder.Header(headerColor);

	ImGui::Dummy(ImVec2(0, 3));

	ImGui::TextUnformatted(title);

	ImGui::SameLine(0, nodeWidth - ImGui::GetItemRectSize().x - 25.0f);

	auto icon = node->layer->IsEnable() ? ICON_FA_PAUSE : ICON_FA_PLAY;
	auto btnColor = node->layer->IsEnable() ? ImColor(128, 128, 128) : ImColor(128,195,255);

	ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImVec4(btnColor));
	if (ImGui::Button(icon))
	{
		node->layer->SetEnable(!node->layer->IsEnable());
	}
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, 3));
	builder.EndHeader();
}

void AnimatorEditorTab::RenderNode(Node* node)
{
	namespace util = ax::NodeEditor::Utilities;

	//void* ptr = nullptr;
	//auto type = GetNodeType(node, &ptr);

	util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
	builder.Begin(node->nodeId);
	node->Render(builder);
	builder.End();

	if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
	{
		OnGraphNodeDoubleClicked(node);
	}
}

void AnimatorEditorTab::RenderModelSkeleton()
{
	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		/*if (m_renderSkeletonOffset == Vec3::ZERO)
		{
			m_renderSkeletonOffset = -m_object->GetCommittedGlobalTransform().Right().Normal() * 1.0f;
		}*/
		/*auto mat = m_object->GetLocalTransform().ToTransformMatrix();

		debugGraphics->DrawDirection(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
		debugGraphics->DrawDirection(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
		debugGraphics->DrawDirection(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });*/

		auto& model = m_animator->m_model3D;
		auto& offsetMatrix = model->m_boneOffsetMatrixs;
		std::vector<Vec3> bonePos;
		bonePos.reserve(offsetMatrix.size());

		auto& rootTrans = m_object->GetCommittedGlobalTransform();

		for (auto& bone : model->m_boneOffsetMatrixs)
		{
			bonePos.push_back((bone.GetInverse() * rootTrans).Position());
		}

		size_t i = 0;
		auto& nodes = model->m_nodes;
		for (auto& node : nodes)
		{
			auto& modelNode = m_modelNodes[i];
			if (node.boneId != INVALID_ID && node.parentId != INVALID_ID && nodes[node.parentId].boneId != INVALID_ID)
			{
				auto cur = (Vec4(bonePos[node.boneId], 1.0f)).xyz() + m_renderSkeletonOffset;
				auto parent = (Vec4(bonePos[nodes[node.parentId].boneId], 1.0f)).xyz() + m_renderSkeletonOffset;

				Vec4 color = { 0,1,0,1 };

				float percent = ((i % 3) + 1);
				color[0] /= percent;
				color[1] /= percent;
				color[2] /= percent;

				if (modelNode->isSelected)
				{
					color = { 1,1,0,1 };
				}

				debugGraphics->DrawLineSegment(parent, cur, color, 0.01f);
			}
			i++;
		}
	}
}

void AnimatorEditorTab::RenderModelNodeHierarchy(void (*callback)(ModelNode*, void*), void* userPtr)
{
	auto& model = m_animator->m_model3D;
	auto& nodes = model->m_nodes;

	ModelNode* rootBone = nullptr;
	for (auto& node : m_modelNodes)
	{
		if (nodes[node->nodeIdx].boneId != INVALID_ID)
		{
			rootBone = node;
			break;
		}
	}

	if (rootBone)
		RenderModelNodeHierarchyImpl(callback, userPtr, rootBone, nullptr);
}

void AnimatorEditorTab::RenderModelNodeHierarchyImpl(void (*callback)(ModelNode*, void*), void* userPtr, ModelNode* modelNode, void* outRect)
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

	auto open = ImGui::TreeNodeEx((void*)modelNode, nodeFlags, name.c_str());
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

	ImGui::SameLine(); callback(modelNode, userPtr);

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

void AnimatorEditorTab::OnGraphNodeDoubleClicked(Node* node)
{
	if (node->layerType == LAYER_TYPE::MIXING)
	{
		SetTPoseMode(true);
	}
}

void AnimatorEditorTab::CreateLink(Node* src, Node* dest, ID destInputId)
{
	assert(
		std::find_if(src->outputLinks.begin(), src->outputLinks.end(),
			[=](const Link* link)
			{
				return link->dest == dest;
			}
		) == src->outputLinks.end()
	);

	assert(dest->inputs[destInputId].link == nullptr);

	auto link = std::make_unique<Link>();
	link->linkId = GetNextId();
	link->dest = dest;
	link->destIdx = destInputId;
	link->src = src;

	dest->inputs[destInputId].link = link.get();
	link->srcIdx = src->outputLinks.size();
	src->outputLinks.push_back(link.get());

	m_links.push_back(std::move(link));
}

void AnimatorEditorTab::DeleteLink(ID linkId)
{
	auto it = std::find_if(m_links.begin(), m_links.end(),
		[=](const UniquePtr<Link>& link)
		{
			return link->linkId == linkId;
		}
	);

	auto link = std::move(*it);
	m_links.erase(it);

	link->dest->inputs[link->destIdx].link = nullptr;

	link->src->outputLinks.erase(link->src->outputLinks.begin() + link->srcIdx);
	for (size_t i = 0; i < link->src->outputLinks.size(); i++)
	{
		link->src->outputLinks[i]->srcIdx = i;
	}
}

void AnimatorEditorTab::SetTPoseMode(bool isOn)
{
	m_animator->SetRunning(!isOn);

	if (isOn)
	{
		m_tposeMode = 2;
	}
	else
	{
		m_tposeMode = 0;
	}
	m_isEnableTPoseMode = isOn;
	((AnimatorEditorTPoseLayer*)m_tposeLayer)->m_once = true;
}

AnimatorEditorTab::Node* AnimatorEditorTab::FindNode(ID pinId)
{
	for (auto& node : m_nodes)
	{
		for (auto& input : node->inputs)
		{
			if (input.pinId == pinId)
			{
				return node.get();
			}
		}

		if (node->outputPinId == pinId)
		{
			return node.get();
		}
	}

	return nullptr;
}

void AnimatorEditorTab::RenderBluePrintPanel()
{
	namespace util = ax::NodeEditor::Utilities;

	ed::SetCurrentEditor(m_nodeEditorCtx);
	ed::Begin("Node Editor", ImVec2(0.0, 0.0f));

	auto openPopupPosition = ImGui::GetMousePos();

	ed::Suspend();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	//ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowMinSize, ImVec2(200, 0));

	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node");
	}

	if (ImGui::BeginPopup("Create New Node"))
	{
		ImGui::Dummy({ 100,0 });

		if (ImGui::BeginMenu("Add"))
		{
			UniquePtr<Node> node = nullptr;
			if (ImGui::MenuItem("AnimPlayerLayer"))
			{
				node = std::move(CreateNode(CreateLayer(LAYER_TYPE::ANIMATON_PLAYER)));
			}

			if (ImGui::MenuItem("AnimBlendLayer"))
			{
				node = std::move(CreateNode(CreateLayer(LAYER_TYPE::BLENDING)));
			}

			if (ImGui::MenuItem("AnimMixLayer"))
			{
				node = std::move(CreateNode(CreateLayer(LAYER_TYPE::MIXING)));
			}

			if (node.get())
			{
				m_nodes.push_back(std::move(node));

				ed::SetCurrentEditor(m_nodeEditorCtx);

				auto raw = m_nodes.back().get();
				ed::SetNodePosition(raw->nodeId, openPopupPosition);
			}

			ImGui::EndMenu();
		}

		ImGui::Dummy({ 100,0 });
		ImGui::EndPopup();
	}
	//ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ed::Resume();

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
					auto src = FindNode(ID(inputPinId));
					auto dest = FindNode(ID(outputPinId));

					if (dest->outputPinId == ID(outputPinId) && src->outputPinId == ID(inputPinId))
					{
						// both are outputs
						goto end;
					}

					if (dest->outputPinId != ID(outputPinId) && src->outputPinId != ID(inputPinId))
					{
						// both are inputs
						goto end;
					}

					if (dest->outputPinId == ID(outputPinId))
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
							break;
						}
					}
					assert(destIdx != INVALID_ID);

					if (dest->inputs[destIdx].link != nullptr)
					{
						DeleteLink(dest->inputs[destIdx].link->linkId);
					}

					CreateLink(src, dest, destIdx);
				}
			}
		}

	}

end:
	ed::EndCreate();

	for (auto& link : m_links)
	{
		ed::Link(link->linkId, link->src->outputPinId, link->dest->inputs[link->destIdx].pinId, { 1,1,1,1 }, 2);
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

void AnimatorEditorTab::WaitForDoneBuilding()
{
	if (!m_isBuilding)
	{
		return;
	}

	TaskSystem::WaitForHandle(&m_builtWaitingHandle);

	m_isBuilding = false;
	assert(m_buildingNow == m_buildingTotal);
}

void AnimatorEditorTab::BuildGraph()
{
	if (m_isBuilding)
	{
		return;
	}

	Task task;
	task.Params() = this;
	task.Entry() = [](void* p)
	{
		((AnimatorEditorTab*)p)->BuildGraphImpl();
	};

	m_buildingNow = 0;
	m_buildingTotal = m_nodes.size() + 1;

	TaskSystem::PrepareHandle(&m_builtWaitingHandle);
	TaskSystem::Submit(&m_builtWaitingHandle, task, Task::LOW);
}

void AnimatorEditorTab::BuildGraphImpl()
{
	String errDesc;
	auto count = m_nodes.size();
	int errCount = 0;

	Node* root = nullptr;
	
	// check single graph
	int outputNodeCount = 0;
	{
		for (size_t i = 0; i < count; i++)
		{
			if (m_isRequestClosing)
			{
				break;
			}

			auto node = m_nodes[i].get();
			if (node->outputLinks.size() == 0)
			{
				outputNodeCount++;
				root = node;
			}
		}

		if (outputNodeCount != 1)
		{
			std::cerr << "[BUILD]: Failed. Graph must have only 1 output node.";
			errCount++;
		}
	}

	// check graph has no loop
	if (outputNodeCount == 1)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (m_isRequestClosing)
			{
				break;
			}

			auto node = m_nodes[i].get();
			node->visited = 0;
			node->excutionOrder = 0;
		}

		std::queue<Node*> queue;
		root->visited = 1;
		queue.push(root);

		bool hasLoop = false;

		while (!queue.empty())
		{
			if (m_isRequestClosing)
			{
				break;
			}

			auto top = queue.front();
			queue.pop();

			assert(top->visited == 1);

			top->visited = 2;

			for (auto& input : top->inputs)
			{
				if (input.link == nullptr)
				{
					continue;
				}

				if (input.link->src->visited == 0)
				{
					input.link->src->visited = 1;
					queue.push(input.link->src);
				}

				if (input.link->src->visited > 1)
				{
					hasLoop = true;
				}

				input.link->src->excutionOrder = top->excutionOrder + 1;
			}

			if (hasLoop)
			{
				break;
			}
		}

		if (hasLoop)
		{
			std::cerr << "[BUILD]: Failed. Graph has a loop.";
			errCount++;
		}
	}

	for (size_t i = 0; i < count; i++)
	{
		if (m_isRequestClosing)
		{
			break;
		}

		auto node = m_nodes[i].get();
		auto errCode = node->ValidateBeforeBuilt(errDesc);
		if (errCode != 0)
		{
			std::cerr << "[BUILD]: Failed. Error " << errCode << ", " << errDesc << "\n";
			errCount++;
		}

		m_buildingNow++;
	}

	if (errCount == 0)
	{
		PlaceNodesToAnimatorLayers();

		m_lastBuildCode = 1;
	}
	else
	{
		m_lastBuildCode = 2;
	}

	//m_buildingNow = m_buildingTotal;
}

void AnimatorEditorTab::PlaceNodesToAnimatorLayers()
{
	std::vector<Node*> nodes;
	for (auto& node : m_nodes)
	{
		nodes.push_back(node.get());
	}

	std::sort(nodes.begin(), nodes.end(), 
		[](const Node* a, const Node* b) 
		{
			return a->excutionOrder > b->excutionOrder;
		}
	);

	std::vector<AnimLayer*> layers;
	size_t count = 0;
	for (auto& node : nodes)
	{
		layers.push_back(node->layer);
		node->layerIdx = count;
		count++;
	}

	//m_animator->m_animLayers.swap(layers);

	auto sys = m_scene->GetAnimationSystem();
	auto runner = sys->AsyncTaskRunner();
	MAIN_SYSTEM_TASK_EXT_1(
		sys, m_animator.Get(), AnimationSystem, AsyncTaskRunner, layers,
		{
			self->m_animator->m_animLayers.swap(layers);
			for (auto& node : self->m_nodes)
			{
				node->ProcessSetInputLayers();
			}

			self->OnBuildNodesDone();

			self->m_buildingNow = self->m_buildingTotal;
		}
	);
}
