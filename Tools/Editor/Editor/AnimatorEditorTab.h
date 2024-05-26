#pragma once
#include "EditorTab.h"

#include "Resources/Texture2D.h"

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
			BLENDING
		};
	};

	struct Node;

	struct NodeExternData
	{
	public:
		AnimatorEditorTab* tab = nullptr;
		Node* node = nullptr;

		inline virtual ~NodeExternData() {};
		virtual void OnNodesUpdated() = 0;

	};

	struct Node
	{
		struct Input
		{
			ID pinId;
			ID linkId;
			Node* node;
		};

		ID nodeIdx = INVALID_ID;
		ID nodeId = INVALID_ID;

		LAYER_TYPE::TYPE layerType = LAYER_TYPE::NONE;
		AnimLayer* layer = nullptr;

		std::vector<Input> inputs;
		ID outputPinId = INVALID_ID;

		NodeExternData* externData = nullptr;

		inline ~Node()
		{
			if (externData)
			{
				delete externData;
				externData = nullptr;
			}
		}
	};

	struct Link
	{
		ID linkId;

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

	String m_modelPath;
	Handle<GameObject> m_object;
	Handle<GameObject> m_cam;
	Handle<ClassMetadata> m_objMetadata;
	Handle<AnimatorSkeletalArray> m_animator;

	ax::NodeEditor::EditorContext* m_nodeEditorCtx = nullptr;

	ID m_onSaveListenerId = INVALID_ID;

	Resource<Texture2D> m_nodeHeaderTexture;

	ID m_nextId = 0;
	std::vector<Link> m_links;
	std::map<ID, Node*> m_pinIdToNode;

	std::vector<UniquePtr<Node>> m_nodes;
	std::vector<Vec2> m_savedPositions;

	char m_inputName[256] = {};

	std::vector<AnimationEditingState> m_animationsEditingState;

	byte m_isFirstRender = 0;

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

	void WriteNodeDataToJson(Serializer* serializer, json& j) const;
	void ReadNodeDataFromJson(Serializer* serializer, const json& j);

	void BuildNodesFromAnimator();

	LAYER_TYPE::TYPE GetNodeType(Node* node, void** concretePtr);
	LAYER_TYPE::TYPE GetLayerType(AnimLayer* layer);

	UniquePtr<Node> CreateNode(AnimLayer* layer);

	std::vector<AnimLayer*> GetInputLayers(AnimLayer* layer);
	void BuildNode(Node* node, NodesBuilder& builder);

	void RenderNodeHeader(void*, Node* node, const char* title);
	void RenderNode_ANIMATON_PLAYER(Node* node, void* concretePtr);
	void RenderNode_BLENDING(Node* node, void* concretePtr);
	void RenderNode(Node* node);

	inline ID GetNextId()
	{
		return ++m_nextId;
	}

	inline Node* GetNode(ID pinId)
	{
		return m_pinIdToNode[pinId];
	}

	inline void CreateLink(Node* src, Node* dest, ID destInputId)
	{
		assert(dest->inputs[destInputId].linkId == INVALID_ID);
		
		auto& input = dest->inputs[destInputId];
		input.pinId = GetNextId();
		input.linkId = GetNextId();
		input.node = src;

		m_links.push_back({ input.linkId,src,destInputId,dest });
	}


private:
	void RenderBluePrintPanel();

};

