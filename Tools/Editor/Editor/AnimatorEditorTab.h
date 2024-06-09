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
			ANIMATON_PLAYER,
			BLENDING,
			MIXING,
		};
	};

	struct Link;

	// prefix "commited" mean the last successful build result 
	// eg: inputs -> commitedInputs; <inputs> is what user see on screen graph, <commitedInputs> is what's actually running inside Animator
	struct Node
	{
		struct Input
		{
			ID pinId = INVALID_ID;
			Link* link = nullptr;
		};

		LAYER_TYPE::TYPE layerType = LAYER_TYPE::NONE;
		AnimLayer* layer = nullptr;
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
			inputs.resize(numInput);
			for (auto& input : inputs)
			{
				input.pinId = tab->GetNextId();
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

		inline virtual bool RenderCustomInspector() { return false; };

		inline void Commit()
		{
			committedInputs = inputs;
		}

		inline AnimLayer* GetInputLayerFromNode(ID id)
		{
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
				delete layer;
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

		template <typename Fn>
		inline void ForEach(Fn fn)
		{
			fn(this);

			for (auto& child : children)
			{
				child->ForEach(fn);
			}
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
	std::map<ID, Node*> m_pinIdToNode;

	std::vector<UniquePtr<Node>> m_nodes;
	//std::vector<Vec2> m_savedPositions;

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

	AnimLayer* m_tposeLayer = nullptr;

	byte m_tposeMode = 0;
	bool m_isEnableTPoseMode = false;
	bool m_isEnableModelInTPoseMode = false;

	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_object);
		tracer->Trace(m_cam);
		tracer->Trace(m_objMetadata);
		tracer->Trace(m_animator);
	}

	AnimatorEditorTab(const String& modelPath, Scene* scene);

	// Inherited via EditorTab
	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;
	void OnShow() override;
	void OnHide() override;
	void OnOpen() override;
	void OnClose() override;

	bool IsCloseable() override;

	void WriteNodeDataToJson(Serializer* serializer, json& j) const;
	void ReadNodeDataFromJson(Serializer* serializer, const json& j);

	void BuildNodesFromAnimator();
	void BuildModelHierarchy();
	void OnBuildNodesDone();

	LAYER_TYPE::TYPE GetNodeType(Node* node, void** concretePtr);
	LAYER_TYPE::TYPE GetLayerType(AnimLayer* layer);

	AnimLayer* CreateLayer(LAYER_TYPE::TYPE type);
	UniquePtr<Node> CreateNode(AnimLayer* layer);

	void BuildNode(Node* node, NodesBuilder& builder);

	void RenderNodeHeader(void*, Node* node, const char* title, float nodeWidth);
	void RenderNode(Node* node);
	void RenderModelSkeleton();
	void RenderModelNodeHierarchy();
	void RenderModelNodeHierarchyImpl(ModelNode*);

	inline ID GetNextId()
	{
		return ++m_nextId;
	}

	inline Node* GetNode(ID pinId)
	{
		return m_pinIdToNode[pinId];
	}

	void CreateLink(Node* src, Node* dest, ID destInputId);
	void DeleteLink(ID linkId);

	void SetTPoseMode(bool isOn);

private:
	void RenderBluePrintPanel();
	void WaitForDoneBuilding();
	void BuildGraph();
	void BuildGraphImpl();
	void PlaceNodesToAnimatorLayers();

};

