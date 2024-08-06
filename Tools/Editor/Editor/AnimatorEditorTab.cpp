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
#include "MainSystem/Animation/AnimLayer/AnimTransitLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimJointLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimMixLayer.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Graphics/DebugGraphics.h"

#include "FileChooser.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#include "ImGuiExtern.h"
#include "ExpressionEval/ExpressionEval.h"

#include "EditorFont.h"

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
		auto layer = (AnimPlayerLayer*)this->layer.Get();

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

		auto layer = (AnimPlayerLayer*)this->layer.Get();
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

	virtual String GetCppClassSource() override
	{
		auto ret = String::Format(R"xxx(
	struct {}
	{
		{};
	};

	AnimPlayerLayer* {} = nullptr;
)xxx", nodeName, ANIMATOR_EDITOR_NODE_CPP_EXE_ID, nodeName);

		return ret;
	}

	virtual String GetCppInitializeSource(const String& animatorVarName) override
	{
		auto ret = String::Format(R"xxx(
		{} = (AnimPlayerLayer*)({}->m_animLayers[{}].Get());
)xxx", nodeName, animatorVarName, excutionOrder);

		return ret;
	}
};

struct AnimTransitLayerNode : public AnimatorEditorTab::Node
{
	SharedPtr<Animation> animation = nullptr;
	ID currentAnimId = 0;

	float start = -1;
	float end = -1;
	float fadeTime = 0;
	AnimTransitLayer::TransitDirection::DIRECTION transitDirection = AnimTransitLayer::TransitDirection::DIRECTION::FORWARD;

	inline static const char* TRANSIT_DIRECTION_NAMES[] = {
		"FORWARD",
		"BACKWARD"
	};

	AnimPlayerLayer::EventListener* m_inputPlayerLayerListener = nullptr;
	AnimPlayerLayer* m_inputPlayerLayer = nullptr;
	SharedPtr<Animation> m_inputPlayerLayerPrevAnim = nullptr;
	bool isEnableFadeTimeTest = false;

	bool isNeedRefreshInputNode = false;
	AnimTransitLayer::EventListener* m_endTransitListener = nullptr;

	AnimTransitLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	inline void UpdateCommitedInput()
	{
		auto UpdateInput = [](Node* input)
		{
			auto playerLayerNode = dynamic_cast<AnimPlayerLayerNode*>(input);
			if (playerLayerNode)
			{
				playerLayerNode->currentAnimId = INVALID_ID;
			}
		};

		if (committedInputs[0].link)
			UpdateInput(committedInputs[0].link->src);
	}

	virtual void OnBuiltDone() override
	{
		currentAnimId = 0;
		animation = tab->m_animator->m_model3D->m_animations[currentAnimId];
		auto layer = (AnimTransitLayer*)this->layer.Get();
		auto playerInputLayer = dynamic_cast<AnimPlayerLayer*>(layer->m_input);
		if (playerInputLayer)
		{
			m_inputPlayerLayer = playerInputLayer;
			m_inputPlayerLayerListener = playerInputLayer->AddPlayingListener<MainSystemInfo::RENDERING_ID>(
				this, 0.0f,
				[](Handle<AnimTransitLayerNode> self, AnimTransitLayer* layer)
				{
					auto l0 = dynamic_cast<AnimPlayerLayer*>(layer->m_input);
					if (l0 && self->isEnableFadeTimeTest)
					{
						//l0->SetAnimation();
						layer->FadeTo(self->transitDirection, self->fadeTime, self->animation, self->start, self->end);
					}
				},
				layer
			);
		}

		transitDirection = layer->m_lastFadeState.direction;

		if (m_endTransitListener == nullptr)
		{
			m_endTransitListener = layer->AddEndTransitListener<MainSystemInfo::RENDERING_ID>(
				this,
				[](Handle<AnimTransitLayerNode> self, AnimTransitLayer* layer)
				{
					auto l0 = dynamic_cast<AnimPlayerLayer*>(layer->m_input);
					if (l0 && self->isEnableFadeTimeTest && self->m_inputPlayerLayerPrevAnim)
					{
						l0->SetAnimation(self->m_inputPlayerLayerPrevAnim, -1, -1);
					}

					if (self->isNeedRefreshInputNode)
					{
						self->UpdateCommitedInput();
						self->isNeedRefreshInputNode = false;
					}
				},
				layer
			);
		}
	}

	virtual std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimTransitLayer*)this->layer.Get();
		return {
			layer->m_input
		};
	}

	virtual void ProcessSetInputLayers() override
	{
		auto layer = (AnimTransitLayer*)this->layer.Get();
		layer->m_input = GetInputLayerFromNode(0);

		if (m_inputPlayerLayer && m_inputPlayerLayerListener)
		{
			m_inputPlayerLayer->RemoveListener(m_inputPlayerLayerListener);
			m_inputPlayerLayer = nullptr;
			m_inputPlayerLayerListener = nullptr;
		}
	}

	virtual void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimTransitLayer*)this->layer.Get();
		auto& animations = tab->m_animator->m_model3D->m_animations;

		//util::BlueprintNodeBuilder builder(m_nodeHeaderTexture->GetNativeHandle(), m_nodeHeaderTexture->Width(), m_nodeHeaderTexture->Height());
		//builder.Begin(node->nodeId);
		{
			//ed::SetNodePosition(uniqueId, { 0,0 });

			tab->RenderNodeHeader(&builder, this, "AnimTransitLayer", 250);

			{
				//assert(committedInputs.size() == 2);

				ImGui::BeginGroup();

				{
					auto& input = inputs[0];
					builder.Input(input.pinId);
					ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Circle, false); ImGui::SameLine();
					ImGui::TextUnformatted("Input");
					builder.EndOutput();
				}

				ImGui::EndGroup();
			}

			ImGui::SameLine(); ImGui::Dummy({ 140, 0 }); ImGui::SameLine();

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
					layer->FadeTo(transitDirection, fadeTime, animation, start, end);
					//UpdateCommitedInput();
					isNeedRefreshInputNode = true;
				}
				ImGui::SameLine(); //ImGui::Dummy({ 20, 0 }); ImGui::SameLine();
				ImGui::TextUnformatted("Fade Animation");
				
				ImGui::SetNextItemWidth(250);
				if (ed::BeginNodeCombo("##FadeDirection", TRANSIT_DIRECTION_NAMES[transitDirection], 0))
				{
					for (size_t i = 0; i < sizeof(TRANSIT_DIRECTION_NAMES) / sizeof(TRANSIT_DIRECTION_NAMES[0]); i++)
					{
						if (ImGui::Selectable(TRANSIT_DIRECTION_NAMES[i]))
						{
							transitDirection = (decltype(transitDirection))i;
						}
					}
					ed::EndNodeCombo();
				}

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

				if (committedInputs[0].link)
				{
					auto l0 = dynamic_cast<AnimPlayerLayer*>(committedInputs[0].link->src->layer.Get());
					if (l0)
					{
						ImGui::SetNextItemWidth(250);
						if (ImGui::Checkbox("Test Fade Anim", &isEnableFadeTimeTest))
						{
							m_inputPlayerLayerPrevAnim = l0->m_animation;
						}

						ImGui::BeginDisabled(!isEnableFadeTimeTest);
						ImGui::SetNextItemWidth(160);
						auto v = m_inputPlayerLayerListener->TriggerTick() / l0->m_animation->GetTicksPerSecond();
						if (ImGui::SliderFloat("StartTick", &v, 0.0f, l0->m_animation->GetTickDuration() / l0->m_animation->GetTicksPerSecond()))
						{
							m_inputPlayerLayerListener->TriggerTick() = v * l0->m_animation->GetTicksPerSecond();
						}
						ImGui::EndDisabled();
					}
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

	virtual String GetCppClassSource() override
	{
		auto ret = String::Format(R"xxx(
	struct {}
	{
		{};
	};

	AnimTransitLayer* {} = nullptr;
)xxx", nodeName, ANIMATOR_EDITOR_NODE_CPP_EXE_ID, nodeName);

		return ret;
	}

	virtual String GetCppInitializeSource(const String& animatorVarName) override
	{
		auto ret = String::Format(R"xxx(
		{} = (AnimTransitLayer*)({}->m_animLayers[{}].Get());
)xxx", nodeName, animatorVarName, excutionOrder);

		return ret;
	}
};

struct AnimBlendLayerNode : public AnimatorEditorTab::Node
{
	struct EditType
	{
		enum TYPE
		{
			FIXED_VALUE,
			LINEAR,
			QUADRATIC,
			CUSTOM_FUNCTION
		};

		inline static const char* NAMES[] = {
			"FIXED_VALUE",
			"LINEAR",
			"QUADRATIC",
			"CUSTOM_FUNCTION"
		};
	};

	class FixedFunction1D : public Function1D
	{
	public:
		SERIALIZABLE_CLASS(FixedFunction1D, SERIALIZABLE_MEM_SHARED);

		mutable float m_value = 0;

		virtual float Test(float v) const override
		{
			return m_value;
		}

		void CloneFrom(Serializer* serializer, Serializable* another) override
		{
		}
		void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
		{
		}
		void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
		{
		}
		void SerializeToJson(Serializer* serializer, json& j) const override
		{
			j["FixedValue"] = m_value;
		}
		void DeserializeFromJson(Serializer* serializer, const json& j) override
		{
			m_value = j["FixedValue"];
		}
		Handle<ClassMetadata> GetMetadata(size_t sign) override
		{
			return Handle<ClassMetadata>();
		}
		void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
		{
		}
	};

	class CustomFunction1D : public Function1D
	{
	public:
		SERIALIZABLE_CLASS(CustomFunction1D, SERIALIZABLE_MEM_SHARED);

		String m_exprtStr;
		ExpressionEval1D* m_exprEval = nullptr;

		CustomFunction1D()
		{
			m_exprEval = ExpressionEval1D::New();
		}

		~CustomFunction1D()
		{
			ExpressionEval1D::Delete(m_exprEval);
		}

		virtual float Test(float v) const override
		{
			return m_exprEval->Test(v);
		}

		void SetExpression(const String& exprtStr)
		{
			if (!m_exprEval->SetExpression(exprtStr.c_str()))
			{
				return;
			}

			m_exprtStr = exprtStr;
		}

		// Inherited via Function1D
		void CloneFrom(Serializer* serializer, Serializable* another) override
		{
		}
		void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
		{
		}
		void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
		{
		}
		void SerializeToJson(Serializer* serializer, json& j) const override
		{
			j["ExpressionString"] = m_exprtStr;
		}
		void DeserializeFromJson(Serializer* serializer, const json& j) override
		{
			m_exprtStr = j["ExpressionString"];
			m_exprEval->SetExpression(m_exprtStr.c_str());
		}
		Handle<ClassMetadata> GetMetadata(size_t sign) override
		{
			return Handle<ClassMetadata>();
		}
		void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
		{
		}
	};
		
	SharedPtr<Function1D> m_currentFunction = nullptr;
	SharedPtr<CustomFunction1D> m_customFunction = nullptr;
	SharedPtr<FixedFunction1D> m_fixedFunction = nullptr;
	float m_rangeMin = 0;
	float m_rangeMax = 1;

	EditType::TYPE m_editType = EditType::CUSTOM_FUNCTION;

	char m_textEditBuffer[1024] = {};

	AnimBlendLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{
		m_fixedFunction = std::make_shared<FixedFunction1D>();
		m_customFunction = std::make_shared<CustomFunction1D>();
		m_currentFunction = m_customFunction;
	}

	virtual void OnBuiltDone() override
	{
		auto layer = (AnimBlendLayer*)this->layer.Get();
		auto func = layer->m_controlFunction.get();

		if (dynamic_cast<FixedFunction1D*>(func))
		{
			m_editType = EditType::FIXED_VALUE;
		}
		else if (dynamic_cast<FunctionLinear1D*>(func))
		{
			m_editType = EditType::LINEAR;
		}
		else if (dynamic_cast<FunctionQuadratic1D*>(func))
		{
			m_editType = EditType::QUADRATIC;
		}
		else if (dynamic_cast<CustomFunction1D*>(func))
		{
			m_editType = EditType::CUSTOM_FUNCTION;

			if (m_customFunction.get() != layer->m_controlFunction.get())
			{
				m_customFunction = std::dynamic_pointer_cast<CustomFunction1D>(layer->m_controlFunction);
			}
		}

		if (m_currentFunction.get() != func)
		{
			m_currentFunction = layer->m_controlFunction;
		}
	}

	virtual std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimBlendLayer*)this->layer.Get();
		return {
			layer->m_input[0],
			layer->m_input[1],
		};
	}

	virtual void ProcessSetInputLayers() override
	{
		auto layer = (AnimBlendLayer*)this->layer.Get();
		layer->m_input[0] = GetInputLayerFromNode(0);
		layer->m_input[1] = GetInputLayerFromNode(1);
	}

	virtual void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimBlendLayer*)this->layer.Get();
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

				if (ImGui::ArrowButton("Run", ImGuiDir_::ImGuiDir_Right))
				{
					OnRunBtnClicked();
				}

				ImGui::SetNextItemWidth(250);
				if (ed::BeginNodeCombo("##Type", EditType::NAMES[m_editType], 0))
				{
					for (size_t i = 0; i < sizeof(EditType::NAMES) / sizeof(EditType::NAMES[0]); i++)
					{
						if (ImGui::Selectable(EditType::NAMES[i]))
						{
							m_editType = (decltype(m_editType))i;

							switch (m_editType)
							{
							case AnimBlendLayerNode::EditType::FIXED_VALUE:
								m_currentFunction = m_fixedFunction;
								break;
							case AnimBlendLayerNode::EditType::LINEAR:
								assert(0);
								break;
							case AnimBlendLayerNode::EditType::QUADRATIC:
								assert(0);
								break;
							case AnimBlendLayerNode::EditType::CUSTOM_FUNCTION:
								m_currentFunction = m_customFunction;
								break;
							default:
								break;
							}
						}
					}
					ed::EndNodeCombo();
				}

				ImGui::SetNextItemWidth(200);
				ImGui::DragFloat("Min", &m_rangeMin, 0.001f, -INFINITY, INFINITY);
				ImGui::SetNextItemWidth(200);
				ImGui::DragFloat("Max", &m_rangeMax, 0.001f, -INFINITY, INFINITY);

				m_rangeMin = std::min(m_rangeMin, m_rangeMax);

				builder.Separator();

				switch (m_editType)
				{
				case AnimBlendLayerNode::EditType::FIXED_VALUE:
					EditFixed();
					break;
				case AnimBlendLayerNode::EditType::LINEAR:
					EditLinear();
					break;
				case AnimBlendLayerNode::EditType::QUADRATIC:
					EditQuadratic();
					break;
				case AnimBlendLayerNode::EditType::CUSTOM_FUNCTION:
					EditCustomFunction();
					break;
				default:
					break;
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

	virtual bool RenderCustomInspector() override
	{
		if (ImGui::ArrowButton("Run", ImGuiDir_::ImGuiDir_Right))
		{
			OnRunBtnClicked();
		}

		if (m_editType == EditType::CUSTOM_FUNCTION)
		{
			auto size = ImGui::GetWindowSize();
			ImGui::TextUnformatted("Expression:");
			ImGui::InputTextMultiline("##edit", m_textEditBuffer, sizeof(m_textEditBuffer), { size.x,size.y / 4.0f });
		}
		
		return true;
	}

	virtual String GetCppClassSource() override
	{
		auto ret = String::Format(R"xxx(
	struct {}
	{
		{};
	};

	AnimBlendLayer* {} = nullptr;
)xxx", nodeName, ANIMATOR_EDITOR_NODE_CPP_EXE_ID, nodeName);

		return ret;
	}

	virtual String GetCppInitializeSource(const String& animatorVarName) override
	{
		auto ret = String::Format(R"xxx(
		{} = (AnimBlendLayer*)({}->m_animLayers[{}].Get());
)xxx", nodeName, animatorVarName, excutionOrder);

		return ret;
	}

	inline void OnRunBtnClicked()
	{
		if (m_editType == EditType::CUSTOM_FUNCTION)
		{
			m_customFunction->SetExpression(m_textEditBuffer);
		}

		auto layer = (AnimBlendLayer*)this->layer.Get();
		if (m_editType == EditType::CUSTOM_FUNCTION && m_customFunction->m_exprtStr.empty())
		{
			return;
		}

		layer->SetControlFunction(m_currentFunction, m_rangeMin, m_rangeMax);
	}
	
	inline void EditFixed()
	{
		ImGui::SetNextItemWidth(200);
		ImGui::SliderFloat("Value", &m_fixedFunction->m_value, 0, 1.0f);
	}

	inline void EditLinear()
	{

	}

	inline void EditQuadratic()
	{

	}

	inline void EditCustomFunction()
	{
		if (ImGui::Button(ICON_FA_PEN_TO_SQUARE))
		{
			ed::SelectNode(nodeId);
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("Expression:");
		
		ImGui::SetNextItemWidth(250);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::TextUnformatted(m_textEditBuffer);
		ImGui::PopStyleVar();
	}
};

struct AnimJointLayerNode : public AnimatorEditorTab::Node
{
	std::vector<std::vector<bool>> masks;
	ID selectedInputId = INVALID_ID;

	float editingWeight = 0.0f;

	bool firstLoad = false;

	AnimJointLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	// Inherited via Node
	void OnBuiltDone() override
	{
		if (firstLoad && layerIdx == INVALID_ID)
		{
			return;
		}

		auto layer = (AnimJointLayer*)this->layer.Get();
		masks.resize(layer->m_inputs.size());
		size_t i = 0;
		for (auto& input : layer->m_inputs)
		{
			masks[i] = input.mask;
			i++;
		}
	}

	std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimJointLayer*)this->layer.Get();
		std::vector<AnimLayer*> ret; 
		for (auto& input : layer->m_inputs)
		{
			ret.push_back(input.layer);
		}
		return ret;
	}

	void ProcessSetInputLayers() override
	{
		auto layer = (AnimJointLayer*)this->layer.Get();
		auto& inputs = layer->m_inputs;
		inputs.resize(GetInputLayerFromNodeCount());

		for (size_t i = 0; i < inputs.size(); i++)
		{
			auto& input = inputs[i];
			input.layer = GetInputLayerFromNode(i);
			input.mask = masks[i];
		}
	}

	void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimJointLayer*)this->layer.Get();
		auto& animations = tab->m_animator->m_model3D->m_animations;

		tab->RenderNodeHeader(&builder, this, "AnimJointLayer", 250);

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
				auto& weights = masks[selectedInputId];
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
		auto v = (bool)masks[selectedInputId][node->nodeIdx];
		if (ImGui::Checkbox("", &v))
		{
			masks[selectedInputId][node->nodeIdx] = v;
		}
		ImGui::PopID();
	};

	virtual String GetCppClassSource() override
	{
		auto ret = String::Format(R"xxx(
	struct {}
	{
		{};
	};

	AnimJointLayer* {} = nullptr;
)xxx", nodeName, ANIMATOR_EDITOR_NODE_CPP_EXE_ID, nodeName);

		return ret;
	}

	virtual String GetCppInitializeSource(const String& animatorVarName) override
	{
		auto ret = String::Format(R"xxx(
		{} = (AnimJointLayer*)({}->m_animLayers[{}].Get());
)xxx", nodeName, animatorVarName, excutionOrder);

		return ret;
	}

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
		json["Masks"] = masks;
	}

	void ReadFromJson(const json& json) override
	{
		firstLoad = true;

		if (json.contains("LayerWeights"))
		{
			masks = json["Masks"];
		}
		else
		{
			masks.resize(inputs.size());
			for (auto& w : masks)
			{
				w.resize(this->layer->NodeGlobalTransforms().size(), 1.0f);
				RevalueRootWeights();
			}
		}
	}

	void EmplaceBackInput()
	{
		auto layer = (AnimJointLayer*)this->layer.Get();

		auto& weight = masks.emplace_back();
		weight.resize(layer->NodeGlobalTransforms().size(), 1.0f);
		RevalueRootWeights();

		ResizeInputs(inputs.size() + 1);
	}

	void RevalueRootWeights()
	{
		if (tab->m_isShowRootNode)
		{
			return;
		}

		//const auto VALUE = 1.0f / layerWeights.size();
		auto& nodes = tab->m_animator->m_model3D->m_nodes;
		size_t count = 0;
		for (auto& weights : masks)
		{
			for (size_t i = 0; i < weights.size(); i++)
			{
				auto& node = nodes[i];

				bool isEffectedByBone = false;
				auto cur = i;
				while (cur != INVALID_ID)
				{
					if (nodes[cur].boneId != INVALID_ID)
					{
						isEffectedByBone = true;
						break;
					}
					cur = nodes[cur].parentId;
				}

				if (isEffectedByBone)
				{
					continue;
				}
				weights[i] = (count == 0 ? 1.0f : 0.0f);
			}
			count++;
		}
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
		masks.erase(masks.begin() + selectedInputId);

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

struct AnimMixLayerNode : public AnimatorEditorTab::Node
{
	struct Weight
	{
		ID frameFromLastClick = 0;
		int remainToDeleteCount = 2; 
		float weight = 0;
		ID commitedInputIdx = INVALID_ID;
	};

	std::vector<Weight> m_weights;

	AnimMixLayerNode(AnimatorEditorTab* tab) : AnimatorEditorTab::Node(tab)
	{

	}

	// Inherited via Node
	void OnBuiltDone() override
	{
		auto layer = (AnimMixLayer*)this->layer.Get();
		m_weights.resize(layer->m_inputs.size());
		size_t i = 0;
		for (auto& input : layer->m_inputs)
		{
			m_weights[i].weight = input.weight;
			m_weights[i].commitedInputIdx = i;
			i++;
		}
	}

	std::vector<AnimLayer*> GetInputLayers() override
	{
		auto layer = (AnimMixLayer*)this->layer.Get();
		std::vector<AnimLayer*> ret;
		for (auto& input : layer->m_inputs)
		{
			ret.push_back(input.layer);
		}
		return ret;
	}

	void ProcessSetInputLayers() override
	{
		auto layer = (AnimMixLayer*)this->layer.Get();
		auto& inputs = layer->m_inputs;
		inputs.resize(GetInputLayerFromNodeCount());

		for (size_t i = 0; i < inputs.size(); i++)
		{
			auto& input = inputs[i];
			input.layer = GetInputLayerFromNode(i);
			input.weight = m_weights[i].weight;
			m_weights[i].commitedInputIdx = i;
		}
	}

	void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) override
	{
		namespace util = ax::NodeEditor::Utilities;

		auto layer = (AnimMixLayer*)this->layer.Get();
		auto& animations = tab->m_animator->m_model3D->m_animations;

		tab->RenderNodeHeader(&builder, this, "AnimMixLayer", 285);

		//bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

		{
			ImGui::BeginGroup();

			size_t deleteIdx = INVALID_ID;
			size_t i = 0;
			for (auto& input : inputs)
			{
				auto& weight = m_weights[i];

				builder.Input(input.pinId);
				ax::Widgets::Icon(ImVec2(24, 32), ax::Drawing::IconType::Circle, false); ImGui::SameLine();

				ImVec2 cursor = ImGui::GetCursorPos();
				cursor.x += 0.0f;
				cursor.y += 3.0f;
				ImGui::SetCursorPos(cursor);
				ImGui::TextUnformatted(String::Format("Layer {}", i).c_str());

				ImGui::SameLine(0, 20);
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(50);
				if (ImGui::DragFloat("##edit", &weight.weight, 0.001f, 0.0f, INFINITY))
				{
					if (weight.commitedInputIdx != INVALID_ID)
					{
						layer->m_inputs[weight.commitedInputIdx].weight = weight.weight;
					}
				}
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::PushID(i + inputs.size() + 1);
				if (ImGui::Button(ICON_FA_TRASH))
				{
					weight.frameFromLastClick = 0;
					if ((--weight.remainToDeleteCount) <= 0)
					{
						deleteIdx = i;
					}
				}
				ImGui::PopID();

				builder.EndOutput();
				i++;

				if (weight.frameFromLastClick++ > 100)
				{
					weight.remainToDeleteCount = 2;
				}
			}

			if (i == 0)
			{
				ImGui::Dummy({ 198, 0 });
			}

			if (deleteIdx != INVALID_ID)
			{
				DeleteInput(deleteIdx);
			}

			ImGui::EndGroup();
		}

		ImGui::SameLine(); ImGui::Dummy({ 50, 0 }); ImGui::SameLine();

		{
			builder.Output(outputPinId);
			ax::Widgets::Icon(ImVec2(24, 24), ax::Drawing::IconType::Flow, false);
			builder.EndOutput();
		}

		builder.Separator();

		if (ImGui::Button(ICON_FA_PLUS "  Add Input"))
		{
			EmplaceBackInput();
		}

		/*{
			ImGui::Dummy({ 120, 25 });
		}*/
	}

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

	virtual String GetCppClassSource() override
	{
		auto ret = String::Format(R"xxx(
	struct {}
	{
		{};
	};

	AnimMixLayer* {} = nullptr;
)xxx", nodeName, ANIMATOR_EDITOR_NODE_CPP_EXE_ID, nodeName);

		return ret;
	}

	virtual String GetCppInitializeSource(const String& animatorVarName) override
	{
		auto ret = String::Format(R"xxx(
		{} = (AnimMixLayer*)({}->m_animLayers[{}].Get());
)xxx", nodeName, animatorVarName, excutionOrder);

		return ret;
	}

	void EmplaceBackInput()
	{
		auto layer = (AnimMixLayer*)this->layer.Get();

		auto& weight = m_weights.emplace_back();
		ResizeInputs(inputs.size() + 1);
	}

	void DeleteInput(size_t idx)
	{
		auto& input = inputs[idx];
		if (input.link)
		{
			tab->DeleteLink(input.link->linkId);
		}

		inputs.erase(inputs.begin() + idx);
		m_weights.erase(m_weights.begin() + idx);
	}
};

struct AnimatorEditorTPoseLayer : public AnimLayer
{
public:
	SERIALIZABLE_CLASS(AnimatorEditorTPoseLayer);

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

AnimatorEditorTab::AnimatorEditorTab(const String& modelPath, Scene* scene, const String& tabName)
{
	m_modelPath = modelPath;

	m_nodeHeaderTexture = resource::Load<Texture2D>("Editor/BlueprintBackground.png");

	auto savePath = (EditorContext::GetInstance()->GetSavePath() + "AnimatorEditor/" + tabName + ".ucfg");
	m_edSavePath = savePath;
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
	bool isOpenSettingPopUp = false;

	Animation* deleteAnimation = nullptr;
	ID deleteAnimationId = INVALID_ID;

	if (m_animationsEditingState.size() == 0)
	{
		auto& animations = m_animator->m_model3D->m_animations;
		m_animationsEditingState.resize(animations.size());
		for (size_t i = 0; i < animations.size(); i++)
		{
			auto& state = m_animationsEditingState[i];
			state.name = animations[i]->Name();
		}
	}
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

			//auto mousePos = ImGui::GetMousePos();
			auto windowPos = ImGui::GetWindowPos();
			auto windowSize = ImGui::GetWindowSize();
			if (!m_isHoveringEditName && ImGui::IsMouseHoveringRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y))
				&&
				(ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)
					|| ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Right)
					|| ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Middle)))
			{
				m_renamingNode = nullptr;
			}

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
			[&](auto& node)
			{
				return node->nodeId == ID(nodeId);
			}
		);
		selectedNode = (*it).Get();
	}

	if (selectedNodes.size() == 0)
	{
		if (m_lastDoubleClickNode)
		{
			OnGraphNodeUnDoubleClicked(m_lastDoubleClickNode);
			m_lastDoubleClickNode = nullptr;
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Space))
	{
		std::set<ID> set;
		for (auto& nodeId : selectedNodes)
		{
			set.insert(ID(nodeId));
		}

		for (auto& node : m_nodes)
		{
			if (set.find(node->nodeId) != set.end())
			{
				node->layer->SetEnable(!node->layer->IsEnable());
			}
		}
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
					auto layer = (AnimatorEditorTPoseLayer*)m_tposeLayer.Get();
					layer->m_once = true;
					layer->m_coeff = m_isEnableModelInTPoseMode ? 1.0f : 0.0f;
					layer->Run(0);
					m_animator->UpdateDataToRenderer(m_scene, m_tposeLayer);
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Show Mesh");

			ImGui::SameLine();
			if (ImGui::ToggleButton("ShowRootNodeInTPoseModeToggle", &m_isShowRootNode))
			{
				
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Show Root");

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
			if (ImGui::Button(ICON_FA_FILE_IMPORT " Import Motion"))
			{
				auto path = FileChooser::OpenFileChooser("", false);

				std::vector<Resource<AnimMotion>> motions;
				if (!path.empty() && ResourceUtils::LoadAnimMotion(path, motions) == 0)
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

			ImGui::SameLine();
			if (ImGui::Button("Set Script"))
			{
				ImGui::OpenPopup("Add Script Pop Up");
			}

			if (ImGui::BeginPopup("Add Script Pop Up"))
			{
				auto& components = EditorContext::GetInstance()->m_components[MainSystemInfo::SCRIPTING_ID];
				for (size_t n = 0; n < components.size(); n++)
				{
					auto componentRecord = components[n];
					if (ImGui::Selectable(componentRecord->name.c_str()))
					{
						if (m_object->HasComponent<Script>())
						{
							m_object->RemoveComponentRaw(m_object->GetComponentRaw<Script>());
						}

						m_object->AddComponent(DynamicCast<Script>(componentRecord->ctor()));
					}
				}

				ImGui::EndPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_GEAR " Setting"))
			{
				isOpenSettingPopUp = true;
			}

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_FILE_EXPORT " Export"))
			{
				Export();
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
							if (EditorContext::Get()->IsVariableNameValid(m_inputName))
							{
								state.isEditingName = false;
								if (m_inputName[0])
								{
									state.name = m_inputName;
								}
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

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Delete"))
							{
								deleteAnimation = animation.get();
								deleteAnimationId = i;
							}

							ImGui::EndPopup();
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

	if (deleteAnimation)
	{
		m_animationsEditingState.erase(m_animationsEditingState.begin() + deleteAnimationId);
		m_animator->m_model3D->RemoveAnimation(deleteAnimation);
	}

	if (isOpenSettingPopUp)
	{
		ImGui::OpenPopup("Animator Editor Setting");
	}

	RenderSettingPopup();

	if (m_isExporting && !m_isBuilding)
	{
		ExportImpl();
		m_isExporting = false;
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
				path = tab->GetSaveFilePath();
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
		else
		{
			OnBuildNodesDone();
		}

		m_tposeLayer = m_animator->NewAnimLayer<AnimatorEditorTPoseLayer, true>();
		BuildModelHierarchy();
		return;
	}

	m_object = resource::Load<AnimModel>(m_modelPath)->MakeGameObject();
	m_animator = m_object->GetComponent<AnimatorSkeletalArray>();

	m_tposeLayer = m_animator->NewAnimLayer<AnimatorEditorTPoseLayer, true>();

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

	/*if (m_tposeLayer)
	{
		delete m_tposeLayer;
		m_tposeLayer = nullptr;
	}*/

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
			auto node = m_nodes[i].Get();
			auto pos = ed::GetNodePosition(m_nodes[i]->nodeId);

			json jnode;

			jnode["NodeName"] = node->nodeName;
			jnode["LayerType"] = node->layerType;
			jnode["LayerIdx"] = node->layerIdx;
			jnode["Layer"] = serializer->Serialize(node->layer);

			jnode["NodeId"] = node->nodeId;
			jnode["OutputPinId"] = node->outputPinId;
			jnode["NumInputs"] = node->inputs.size();

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
			json extData;
			node->WriteToJson(extData);
			jnode["NodeExternData"] = extData;

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

	{
		json exportData;
		exportData["ExportName"] = std::string(m_exportInputName);
		exportData["ExportResourcePath"] = m_exportResourcePath;
		exportData["ExportCppPath"] = m_exportCppPath;
		j["ExportData"] = exportData;
	}
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
			Handle<AnimLayer> layer = layerIdx == INVALID_ID ? nullptr : m_animator->m_animLayers[layerIdx];
			if (layer == nullptr)
			{
				serializer->Deserialize(jnode["Layer"], layer);
			}

			auto node = CreateNode(layer);

			node->layerType = layerType;
			node->layerIdx = layerIdx;
			node->nodeId = jnode["NodeId"];
			node->nodeIdx = i;
			node->outputPinId = jnode["OutputPinId"];

			if (jnode.contains("NodeName"))
			{
				node->nodeName = jnode["NodeName"];
			}

			node->ResizeInputs(jnode["NumInputs"]);

			/*json& inputs = jnode["Inputs"];
			for (size_t j = 0; j < inputs.size(); j++)
			{
				auto& jinput = inputs[j];
				Node::Input input;
				input.linkId = jinput["LinkId"];
				input.pinId = jinput["PinId"];
				node->inputs.push_back(input);
			}*/

			if (jnode.contains("NodeExternData"))
			{
				node->ReadFromJson(jnode["NodeExternData"]);
			}
			else
			{
				node->ReadFromJson(jnode);
			}

			Vec2 pos = jnode["Position"];
			ed::SetNodePosition(node->nodeId, ImVec2(pos.x, pos.y));

			nodeIdToNode.insert({ node->nodeId,node.Get() });

			m_nodes.Push(node);
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

	if (j.contains("ExportData"))
	{
		const json& exportData = j["ExportData"];
		std::string name = exportData["ExportName"];
		std::memcpy(m_exportInputName, name.data(), name.length() + 1);

		m_exportResourcePath = exportData["ExportResourcePath"];
		m_exportCppPath = exportData["ExportCppPath"];
	}
}

void AnimatorEditorTab::BuildNodesFromAnimator()
{
	NodesBuilder builder;

	std::map<AnimLayer*, Node*>& animLayerToNode = builder.animLayerToNode;

	auto& layers = m_animator->m_animLayers;
	for (auto& layer : layers)
	{
		m_nodes.Push(CreateNode(layer));

		auto node = m_nodes.back().Get();
		node->layerIdx = m_nodes.size() - 1;
		node->nodeIdx = node->layerIdx;
		animLayerToNode.insert({ layer,node });
	}

	ed::SetCurrentEditor(m_nodeEditorCtx);

	for (auto& node : m_nodes)
	{
		BuildNode(node.Get(), builder);
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
	case AnimatorEditorTab::LAYER_TYPE::PLAYER:
		*concretePtr = dynamic_cast<AnimPlayerLayer*>(node->layer.Get());
		break;
	case AnimatorEditorTab::LAYER_TYPE::TRANSIT:
		*concretePtr = dynamic_cast<AnimTransitLayer*>(node->layer.Get());
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLEND:
		*concretePtr = dynamic_cast<AnimBlendLayer*>(node->layer.Get());
		break;
	case AnimatorEditorTab::LAYER_TYPE::JOINT:
		*concretePtr = dynamic_cast<AnimJointLayer*>(node->layer.Get());
		break;
	case AnimatorEditorTab::LAYER_TYPE::MIX:
		*concretePtr = dynamic_cast<AnimMixLayer*>(node->layer.Get());
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
		type = LAYER_TYPE::PLAYER;
	}
	else if (dynamic_cast<AnimTransitLayer*>(layer))
	{
		type = LAYER_TYPE::TRANSIT;
	}
	else if (dynamic_cast<AnimBlendLayer*>(layer))
	{
		type = LAYER_TYPE::BLEND;
	}
	else if (dynamic_cast<AnimJointLayer*>(layer))
	{
		type = LAYER_TYPE::JOINT;
	}
	else if (dynamic_cast<AnimMixLayer*>(layer))
	{
		type = LAYER_TYPE::MIX;
	}
	else
	{
		assert(0);
	}

	return type;
}

Handle<AnimLayer> AnimatorEditorTab::CreateLayer(LAYER_TYPE::TYPE type)
{
	Handle<AnimLayer> layer = nullptr;

	switch (type)
	{
	case AnimatorEditorTab::LAYER_TYPE::PLAYER:
		layer = m_animator->NewAnimLayer<AnimPlayerLayer, true>();
		break;
	case AnimatorEditorTab::LAYER_TYPE::TRANSIT:
		layer = m_animator->NewAnimLayer<AnimTransitLayer, true>();
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLEND:
		layer = m_animator->NewAnimLayer<AnimBlendLayer, true>();
		break;
	case AnimatorEditorTab::LAYER_TYPE::JOINT:
		layer = m_animator->NewAnimLayer<AnimJointLayer, true>();
		break; 
	case AnimatorEditorTab::LAYER_TYPE::MIX:
		layer = m_animator->NewAnimLayer<AnimMixLayer, true>();
		break;
	default:
		assert(0);
		break;
	}

	//m_animator->m_animLayers.pop_back();

	return layer;
}

Handle<AnimatorEditorTab::Node> AnimatorEditorTab::CreateNode(AnimLayer* layer)
{
	auto layerType = GetLayerType(layer);
	Handle<Node> node;

	switch (layerType)
	{
	case AnimatorEditorTab::LAYER_TYPE::PLAYER:
		node = mheap::New<AnimPlayerLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::TRANSIT:
		node = mheap::New<AnimTransitLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::BLEND:
		node = mheap::New<AnimBlendLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::JOINT:
		node = mheap::New<AnimJointLayerNode>(this);
		break;
	case AnimatorEditorTab::LAYER_TYPE::MIX:
		node = mheap::New<AnimMixLayerNode>(this);
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

	node->nodeName = String::Format("Node_{}", node->nodeId);

	return node;
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

	ImGui::BeginGroup();
	if (m_renamingNode != node)
	{
		//ImGui::Dummy(ImVec2(0, 1));
		ImGui::TextUnformatted(node->nodeName.c_str()); 
		ImGui::Dummy(ImVec2(0, 2.0f));
	}
	else
	{
		ImGui::SetNextItemWidth(nodeWidth - 80.0f);
		if (ImGui::InputText("##EditNodeName", m_renamingNodeNameBuffer, sizeof(m_renamingNodeNameBuffer), ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (ValidateInputNodeName())
			{
				m_renamingNode->nodeName = m_renamingNodeNameBuffer;
				m_renamingNode = nullptr;
			}
		}

		m_isHoveringEditName = ImGui::IsItemHovered();
	}

	ImGui::PushFont(EditorFont::Get()->GetFont(22));
	ImGui::TextUnformatted(title);
	ImGui::PopFont();
	ImGui::EndGroup();

	ImGui::SameLine(0, nodeWidth - ImGui::GetItemRectSize().x - 25.0f - 40.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.5f, 0.7f, 1.0f));
	if (ImGui::Button(ICON_FA_PEN_TO_SQUARE))
	{
		m_renamingNode = node;
		std::memcpy(m_renamingNodeNameBuffer, node->nodeName.c_str(), node->nodeName.length());
		m_renamingNodeNameBuffer[node->nodeName.length()] = '\0';
	}
	ImGui::PopStyleColor();

	ImGui::SameLine();
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

	ModelNode* rootBone = m_root;
	if (!m_isShowRootNode)
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
	ImGui::PushID(ID(modelNode->nodeIdx));
	auto open = ImGui::TreeNodeEx((void*)modelNode, nodeFlags, name.c_str());
	ImGui::PopID();
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

	ImGui::SameLine(); callback(modelNode, userPtr); ImGui::Dummy({ 0, 0 });

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
	m_lastDoubleClickNode = node;

	if (node->layerType == LAYER_TYPE::JOINT)
	{
		SetTPoseMode(true);
	}
}

void AnimatorEditorTab::OnGraphNodeUnDoubleClicked(Node* node)
{
	if (node->layerType == LAYER_TYPE::JOINT)
	{
		//SetTPoseMode(false);
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

	m_deletedLinks.push_back(std::move(link));
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
	((AnimatorEditorTPoseLayer*)m_tposeLayer.Get())->m_once = true;
}

AnimatorEditorTab::Node* AnimatorEditorTab::FindNode(ID pinId)
{
	for (auto& node : m_nodes)
	{
		for (auto& input : node->inputs)
		{
			if (input.pinId == pinId)
			{
				return node.Get();
			}
		}

		if (node->outputPinId == pinId)
		{
			return node.Get();
		}
	}

	return nullptr;
}

void AnimatorEditorTab::RenderBluePrintPanel()
{
	namespace util = ax::NodeEditor::Utilities;

	ed::SetCurrentEditor(m_nodeEditorCtx);
	ed::Begin("Node Editor", ImVec2(0.0, 0.0f));

	for (auto& node : m_nodes)
	{
		RenderNode(node.Get());
	}

	// pop up section
	{
		auto openPopupPosition = ImGui::GetMousePos();

		ed::Suspend();
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		//ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowMinSize, ImVec2(200, 0));

		ed::NodeId& contextNodeId = *(ed::NodeId*)&m_contextNodeId;
		ed::LinkId& contextLinkId = *(ed::LinkId*)&m_contextLinkId;
		if (ed::ShowNodeContextMenu(&contextNodeId))
		{
			ImGui::OpenPopup("Node Context Menu");
		}
		else if (ed::ShowLinkContextMenu(&contextLinkId))
		{
			ImGui::OpenPopup("Link Context Menu");
		}
		else if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
		}
		ed::Resume();

		ed::Suspend();
		if (ImGui::BeginPopup("Create New Node"))
		{
			ImGui::Dummy({ 100,0 });

			if (ImGui::BeginMenu("Add"))
			{
				Handle<Node> node = nullptr;
				if (ImGui::MenuItem("Player"))
				{
					node = CreateNode(CreateLayer(LAYER_TYPE::PLAYER));
				}

				if (ImGui::MenuItem("Blend"))
				{
					node = CreateNode(CreateLayer(LAYER_TYPE::BLEND));
				}

				if (ImGui::MenuItem("Transit"))
				{
					node = CreateNode(CreateLayer(LAYER_TYPE::TRANSIT));
				}

				if (ImGui::MenuItem("Joint"))
				{
					node = CreateNode(CreateLayer(LAYER_TYPE::JOINT));
				}

				if (ImGui::MenuItem("Mix"))
				{
					node = CreateNode(CreateLayer(LAYER_TYPE::MIX));
				}

				if (node)
				{
					node->Commit();
					node->OnBuiltDone();

					m_nodes.Push(node);

					ed::SetCurrentEditor(m_nodeEditorCtx);

					auto& raw = m_nodes.back();
					ed::SetNodePosition(raw->nodeId, openPopupPosition);
				}

				ImGui::EndMenu();
			}

			ImGui::Dummy({ 100,0 });
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Node Context Menu"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				ed::DeleteNode(contextNodeId);
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("Link Context Menu"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				ed::DeleteLink(contextLinkId);
			}

			ImGui::EndPopup();
		}

		//ImGui::PopStyleVar();
		//ImGui::PopStyleVar();
		ed::Resume();
	}

	// create section
	{
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
	}

	// delete section
	{
		if (ed::BeginDelete())
		{
			ed::NodeId nodeId = 0;
			while (ed::QueryDeletedNode(&nodeId))
			{
				if (ed::AcceptDeletedItem())
				{
					auto id = std::find_if(m_nodes.begin(), m_nodes.end(), [nodeId](auto& node) { return node->nodeId == ID(nodeId); });
					if (id != m_nodes.end())
					{
						m_deletedNodes.Push(*id);

						auto& node = *id;
						for (auto& input : node->inputs)
						{
							if (input.link)
							{
								DeleteLink(input.link->linkId);
							}
						}

						for (auto& outputLink : node->outputLinks)
						{
							if (outputLink)
							{
								DeleteLink(outputLink->linkId);
							}
						}

						m_nodes.Remove(id);
					}
				}
			}

			ed::LinkId linkId = 0;
			while (ed::QueryDeletedLink(&linkId))
			{
				if (ed::AcceptDeletedItem())
				{
					auto id = std::find_if(m_links.begin(), m_links.end(), [linkId](auto& link) { return link->linkId == ID(linkId); });
					if (id != m_links.end())
					{
						DeleteLink(id->get()->linkId);
					}
				}
			}
		}
		ed::EndDelete();
	}

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

	m_isBuilding = true;
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

			auto node = m_nodes[i].Get();
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

			auto node = m_nodes[i].Get();
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

		auto node = m_nodes[i].Get();
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
		nodes.push_back(node.Get());
	}

	std::sort(nodes.begin(), nodes.end(), 
		[](const Node* a, const Node* b) 
		{
			return a->excutionOrder > b->excutionOrder;
		}
	);

	for (size_t i = 0; i < nodes.size(); i++)
	{
		auto& node = nodes[i];
		node->excutionOrder = i;
	}

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
			for (auto& node : self->m_nodes)
			{
				node->ProcessSetInputLayers();
			}
			self->m_animator->m_animLayers.clear();
			for (size_t i = 0; i < layers.size(); i++)
			{
				self->m_animator->m_animLayers.Push(layers[i]);
			}

			self->OnBuildNodesDone();
			self->ProcessDeletedNodes();

			self->m_buildingNow = self->m_buildingTotal;
		}
	);
}

void AnimatorEditorTab::ProcessDeletedNodes()
{
	m_deletedNodes.clear();
	m_deletedLinks.clear();

	m_renamingNode = nullptr;
}

bool AnimatorEditorTab::ValidateInputNodeName()
{
	std::string_view nodeName = m_renamingNodeNameBuffer;

	bool ret = EditorContext::Get()->IsVariableNameValid(m_renamingNodeNameBuffer);

	for (auto& node : m_nodes)
	{
		if (node != m_renamingNode && nodeName == node->nodeName.c_str())
		{
			std::cerr << "[ERROR]: Node's name must be unique!\n";

			ret = false;
			break;
		}
	}

	return ret;
}

void AnimatorEditorTab::RenderSettingPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	auto viewPortSize = ImGui::GetMainViewport()->Size;
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(viewPortSize.x / 2, viewPortSize.y / 2));
	if (!ImGui::BeginPopupModal("Animator Editor Setting", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		return;
	}

	if (ImGui::BeginTable("Exports", 2, ImGuiTableFlags_SizingFixedFit))
	{
		auto secondColumnWidth = 0.7f * ImGui::GetWindowWidth();
		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Export Name"); ImGui::SameLine();

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(secondColumnWidth);
			ImGui::InputText("## Edit export name", m_exportInputName, sizeof(m_exportInputName));
		}

		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Export Resource Path"); ImGui::SameLine();

			ImGui::TableNextColumn();
			Accessor temp = Accessor::ForString("Path", m_exportResourcePath, nullptr);
			Variant var = Variant(VARIANT_TYPE::STRING_PATH);
			var.AsString() = m_exportResourcePath;
			DataInspector::InspectStringPathEx(nullptr, temp, var, "Export Resource Path", true, secondColumnWidth, true);
		}

		{
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Export C++ Path"); ImGui::SameLine();

			ImGui::TableNextColumn();
			Accessor temp = Accessor::ForString("Path", m_exportCppPath, nullptr);
			Variant var = Variant(VARIANT_TYPE::STRING_PATH);
			var.AsString() = m_exportCppPath;
			DataInspector::InspectStringPathEx(nullptr, temp, var, "Export C++ Path", true, secondColumnWidth, true);
		}

		ImGui::EndTable();
	}
	//ImGui::EndGroup();

	ImGui::Separator();

	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - 50, ImGui::GetWindowHeight() - 40));

	if (ImGui::Button("OK", ImVec2(100, 0)))
	{
		if (ValidateSetting())
		{
			ImGui::CloseCurrentPopup();
		}
	}

	ImGui::EndPopup();
}

bool AnimatorEditorTab::ValidateSetting()
{
	bool ret = EditorContext::Get()->IsVariableNameValid(m_exportInputName);
	if (!ret)
	{
		std::cerr << "[ERROR]: Export Name illegal!\n";
	}

	return ret;
}

void AnimatorEditorTab::Export()
{
	if (m_isExporting)
	{
		return;
	}

	if (!ValidateSetting())
	{
		return;
	}

	m_isExporting = true;

	BuildGraph();
}

void AnimatorEditorTab::ExportImpl()
{
	// export resource
	{
		auto script = m_object->GetComponent<Script>();
		if (script)
			m_object->RemoveComponent(script);

		Serializer serializer;
		serializer.SetRootUUID(m_object->GetUUID());
		serializer.Serialize(m_object);
		serializer.WriteToFile(m_exportResourcePath + m_exportInputName + ".json");

		if (script)
			m_object->AddComponent(script);
	}
	
	// export cpp
	{
		String cpp = R"xxx(#pragma once

#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimTransitLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimJointLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimMixLayer.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/Utils/Animation.h"

namespace soft
{
	class AnimatorSkeletalArray;
	class AnimPlayerLayer;
	class AnimBlendLayer;
	class AnimTransitLayer;
	class AnimMixLayer;
	class AnimJointLayer;
	class Animation;
}

using namespace soft;

struct )xxx";

		cpp = cpp + m_exportInputName;
		cpp = cpp + "\n{\n";

		String animationIdStr = "";
		for (size_t i = 0; i < m_animationsEditingState.size(); i++)
		{
			auto& state = m_animationsEditingState[i];
			animationIdStr = animationIdStr + String::Format("\t\tconstexpr static ID {} = {};\n", state.name, i);
		}

		String animationsStr = "";
		for (size_t i = 0; i < m_animationsEditingState.size(); i++)
		{
			auto& state = m_animationsEditingState[i];
			animationsStr = animationsStr + String::Format("\t\tSharedPtr<Animation> {};\n", state.name);
		}

		String animationsInitializeStr = "";
		for (size_t i = 0; i < m_animationsEditingState.size(); i++)
		{
			auto& state = m_animationsEditingState[i];
			animationsInitializeStr = animationsInitializeStr + String::Format("\t\tAnimations.{} = animator->m_model3D->GetAnimation({});\n", state.name, i);
		}

		cpp = cpp + String::Format(R"(
	struct AnimationID
	{

{}
	};

	struct _Animations
	{

{}
	};

	_Animations Animations;
)", animationIdStr, animationsStr);

		for (auto& node : m_nodes)
		{
			cpp = cpp + node->GetCppClassSource() + "\n";
		}

		String initStr = "";
		for (auto& node : m_nodes)
		{
			initStr = initStr + node->GetCppInitializeSource("animator");
		}

		cpp += String::Format(R"(
	inline void Initialize(AnimatorSkeletalArray* animator)
	{
{}
		{}
	}
)", animationsInitializeStr, initStr);

		cpp = cpp + "\n};\n";

		FileUtils::WriteFile((m_exportCppPath + m_exportInputName + ".h").c_str(), cpp.c_str(), cpp.length());
	}
}

void AnimatorEditorTab::InitializeSerializableList()
{
	SerializableDB::Get()->Register<AnimBlendLayerNode::CustomFunction1D>();
	SerializableDB::Get()->Register<AnimBlendLayerNode::FixedFunction1D>();
}