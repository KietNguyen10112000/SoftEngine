#pragma once
#include "EditorTab.h"

#include "Resources/Texture2D.h"

#include "NodeEditorUtils/builders.h"

#include "MainSystem/Animation/AnimLayer/AnimLayer.h"

namespace soft
{
	class GameObject;
	class AnimatorSkeletalArray;
	class Scene;
	class AnimLayer;
}

namespace ax
{
	namespace NodeEditor
	{
		struct EditorContext;
	}
}

class AnimatorEditorTab : public EditorTab
{
public:
	struct LAYER_TYPE
	{
		enum TYPE
		{
			NONE,
			PLAYER,
			TRANSIT,
			BLEND,
			JOINT,
			MIX,
		};
	};

	struct Link;
	struct ModelNode;

#define ANIMATOR_EDITOR_NODE_CPP_EXE_ID String::Format("constexpr static ID ID = {}", excutionOrder)

	class TPoseLayer : public AnimLayer
	{
	public:
		virtual void SetTransform(const Transform& transform) = 0;
		virtual Transform GetTransform() const = 0;

	};

	// prefix "commited" mean the last successful build result 
	// eg: inputs -> commitedInputs; <inputs> is what user see on screen graph, <commitedInputs> is what's actually running inside Animator
	struct Node
	{
		struct Input
		{
			ID pinId = INVALID_ID;
			Link* link = nullptr;
		};

		String nodeName = "";

		LAYER_TYPE::TYPE layerType = LAYER_TYPE::NONE;
		Handle<AnimLayer> layer = nullptr;
		ID layerIdx = INVALID_ID;

		AnimatorEditorTab* tab = nullptr;
		ID nodeId = INVALID_ID;
		ID nodeIdx = INVALID_ID;
		std::vector<Input> inputs;
		std::vector<Input> committedInputs;

		ID outputPinId = INVALID_ID;
		std::vector<Link*> outputLinks;

		size_t excutionOrder = INVALID_ID;
		byte visited = 0;

		TRACEABLE_FRIEND();
		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(layer);
		}

		inline Node(AnimatorEditorTab* tab) : tab(tab)
		{
			Commit();
		}

		/*inline Input& EmplaceBackNewInput()
		{
			auto& ret = inputs.emplace_back();
			ret.pinId = tab->GetNextId();
			return ret;
		}*/

		inline void ResizeInputs(size_t numInput)
		{
			auto startIdx = inputs.size();
			inputs.resize(numInput);
			for (auto i = startIdx; i < inputs.size(); i++)
			{
				inputs[i].pinId = tab->GetNextId();
			}
		}

		virtual void OnBuiltDone() = 0;
		virtual std::vector<AnimLayer*> GetInputLayers() = 0;

		virtual void ProcessSetInputLayers() = 0;

		virtual void Render(ax::NodeEditor::Utilities::BlueprintNodeBuilder& builder) = 0;

		// called when user create a link
		virtual int ValidateNewInput(Node* input, ID inputIdx, String& errDesc) = 0;

		// called when user hit build btn
		virtual int ValidateBeforeBuilt(String& errDesc) = 0;

		virtual void WriteToJson(json& json) const = 0;
		virtual void ReadFromJson(const json& json) = 0;

		virtual String GetCppClassSource() = 0;
		virtual String GetCppInitializeSource(const String& animatorVarName) = 0;

		inline virtual bool RenderCustomInspector() { return false; };
		inline virtual void RenderCustomModelTreeNode(ModelNode* node) {};

		inline void Commit()
		{
			committedInputs = inputs;
		}

		inline AnimLayer* GetInputLayerFromNode(ID id)
		{
			if (inputs[id].link == nullptr)
			{
				return nullptr;
			}

			return inputs[id].link->src->layer;
		}

		inline size_t GetInputLayerFromNodeCount()
		{
			return inputs.size();
		}

		inline virtual ~Node()
		{
			if (layerIdx == INVALID_ID && layer)
			{
				//delete layer;
				layer = nullptr;
				layerType = LAYER_TYPE::NONE;
			}
		}
	};

	struct Link
	{
		ID linkId;

		ID srcIdx;
		Node* src;

		ID destIdx;
		Node* dest;
	};

	struct NodesBuilder
	{
		std::map<AnimLayer*, Node*> animLayerToNode;
	};

	struct AnimationEditingState
	{
		bool isEditingName = false;
		String name;
	};

	struct ModelNode
	{
		ModelNode* parent = nullptr;
		std::vector<ModelNode*> children;

		ID nodeIdx = INVALID_ID;
		bool isSelected = false;
		bool isTryingExpand = false;
		bool isOpen = false;

		template <typename Fn>
		inline void ForEach(Fn fn)
		{
			fn(this);

			for (auto& child : children)
			{
				child->ForEach(fn);
			}
		}

		inline auto& Children()
		{
			return children;
		}
	};

	String m_modelPath;
	Handle<GameObject> m_object;
	Handle<GameObject> m_cam;
	Handle<ClassMetadata> m_objMetadata;
	Handle<AnimatorSkeletalArray> m_animator;

	ax::NodeEditor::EditorContext* m_nodeEditorCtx = nullptr;

	ID m_onSaveListenerId = INVALID_ID;

	Resource<Texture2D> m_nodeHeaderTexture;

	ID m_nextId = 0;
	std::vector<UniquePtr<Link>> m_links;
	std::vector<UniquePtr<Link>> m_deletedLinks;
	std::map<ID, Node*> m_pinIdToNode;

	Array<Handle<Node>> m_nodes;
	Array<Handle<Node>> m_deletedNodes;

	char m_inputName[256] = {};

	std::vector<AnimationEditingState> m_animationsEditingState;

	// model's hierarchy root node
	ModelNode* m_root = nullptr;
	std::vector<ModelNode*> m_modelNodes;
	Vec3 m_renderSkeletonOffset = {};

	byte m_isFirstRender = 0;

	bool m_isRequestClosing = false;
	bool m_isBuilding = false;

	// 0: not build yet, 1: success, 2: failed
	byte m_lastBuildCode = 0;
	size_t m_buildingNow = 0;
	size_t m_buildingTotal = 1000;
	TaskWaitingHandle m_builtWaitingHandle = { 0,0 };

	Handle<AnimLayer> m_tposeLayer = nullptr;

	byte m_tposeMode = 0;
	bool m_isEnableTPoseMode = false;
	bool m_isEnableModelInTPoseMode = false;
	bool m_isShowRootNode = false;

	String m_edSavePath;

	Node* m_lastDoubleClickNode = nullptr;
	ID m_contextNodeId = 0;
	ID m_contextLinkId = 0;
	Node* m_renamingNode = nullptr;
	char m_renamingNodeNameBuffer[KB] = {};
	bool m_isHoveringEditName = false;

	String m_exportResourcePath = "./";
	String m_exportCppPath = "./"; 
	char m_exportInputName[256] = {};
	bool m_isExporting = false;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_object);
		tracer->Trace(m_cam);
		tracer->Trace(m_objMetadata);
		tracer->Trace(m_animator);

		tracer->Trace(m_nodes);
		tracer->Trace(m_deletedNodes);
		tracer->Trace(m_tposeLayer);
	}

	AnimatorEditorTab(const String& modelPath, Scene* scene, const String& tabName);

	// Inherited via EditorTab
	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;
	void OnShow() override;
	void OnHide() override;
	void OnOpen() override;
	void OnClose() override;

	inline virtual String GetTabClassName() const override
	{
		return "AnimatorEditor";
	}

	bool IsCloseable() override;

	void WriteNodeDataToJson(Serializer* serializer, json& j) const;
	void ReadNodeDataFromJson(Serializer* serializer, const json& j);

	void BuildNodesFromAnimator();
	void BuildModelHierarchy();
	void OnBuildNodesDone();

	LAYER_TYPE::TYPE GetNodeType(Node* node, void** concretePtr);
	LAYER_TYPE::TYPE GetLayerType(AnimLayer* layer);

	Handle<AnimLayer> CreateLayer(LAYER_TYPE::TYPE type);
	Handle<Node> CreateNode(AnimLayer* layer);

	void BuildNode(Node* node, NodesBuilder& builder);

	void RenderNodeHeader(void*, Node* node, const char* title, float nodeWidth);
	void RenderNode(Node* node);
	void RenderModelSkeleton();
	void RenderModelNodeHierarchy(void (*)(ModelNode*, void*), void* userPtr);
	void RenderModelNodeHierarchyImpl(void (*)(ModelNode*, void*), void* userPtr, ModelNode*, void* outRect);

	void OnGraphNodeDoubleClicked(Node* node);
	void OnGraphNodeUnDoubleClicked(Node* node);

	inline ID GetNextId()
	{
		return ++m_nextId;
	}

	Node* FindNode(ID pinId);

	void CreateLink(Node* src, Node* dest, ID destInputId);
	void DeleteLink(ID linkId);

	void SetTPoseMode(bool isOn);

private:
	void RenderBluePrintPanel();
	void WaitForDoneBuilding();
	void BuildGraph();
	void BuildGraphImpl();
	void PlaceNodesToAnimatorLayers();
	void ProcessDeletedNodes();
	bool ValidateInputNodeName();
	void RenderSettingPopup();
	bool ValidateSetting();
	void Export(); 
	void ExportImpl();

public:
	static void InitializeSerializableList();

public:
	static Handle<TPoseLayer> MakeTPoseLayer(AnimatorSkeletalArray* animator);

};

