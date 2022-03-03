#pragma once

#include <mutex>

#include "Core/Templates/ReusableVector.h"

#include "Math/Math.h"
#include "Core/ILightSystem.h"

class IRenderableObject;
class ICamera;

namespace Math
{
class Rect2D;
class AARect2D;
class Trapezoid;
class Circle;

class Frustum;
class AABB;
class Sphere;
class Box;
};


class Scene;
class SceneQueryContext;
class Engine;

class SceneNodeBlob
{
public:
	struct CameraBuffer
	{
		//Mat4x4 view;
		//use inv of transform as view
		Mat4x4 proj;
	};

	struct Blob
	{
		void* none = 0;
		Mat4x4* transform;
		Light* light;
		CameraBuffer* camera;
	};

public:
	void* m_mem = 0;
	size_t m_memId = 0;

public:
	Light& AsLight() 
	{ 
		return (*((ReusableVector<Light>*)(m_mem)))[m_memId]; 
	};

	/*Mat4x4& AsTransform() 
	{ 
		return (*((ReusableVector<Mat4x4>*)(m_mem)))[m_memId]; 
	};*/

	CameraBuffer& AsCamera() 
	{ 
		return (*((ReusableVector<CameraBuffer>*)(m_mem)))[m_memId]; 
	};

	template <typename T>
	inline T& As()
	{
		return (*((ReusableVector<T>*)(m_mem)))[m_memId];
	};

public:
	template <typename T>
	inline void FreeAs()
	{
		ReusableVector<T>& vec = *(ReusableVector<T>*)m_mem;
		vec.Return(m_memId);
	}

};


#define  SCENE_NODE_DEFINE_TRAVERSE												\
template <typename _Func1, typename _Func2, typename ..._Args> 					\
void _TransformTraverse(const Mat4x4& globalTransform, 							\
	_Func1 prevCall, _Func2 postCall, _Args&&... args);							\
template <typename _Func1, typename _Func2, typename ..._Args>					\
void TransformTraverse(_Func1 prevCall, _Func2 postCall, _Args&&... args);


#define SCENE_NODE_IMPL_TRAVERSE(className, _child)												\
template <typename _Func1, typename _Func2, typename ..._Args> 					\
void className::_TransformTraverse(const Mat4x4& globalTransform, 							\
	_Func1 prevCall, _Func2 postCall, _Args&&... args)							\
{																				\
	Mat4x4 temp = globalTransform * m_transform;								\
	bool recurCall = true;														\
	if constexpr (!(std::is_same_v<std::decay_t<_Func1>, std::nullptr_t>))		\
	{																			\
		recurCall = prevCall(this, temp, std::forward<_Args>(args) ...);		\
	}																			\
	if (recurCall)																\
	for (auto& child : m_childs)												\
	{																			\
		_child->_TransformTraverse(temp, prevCall, postCall, std::forward<_Args>(args) ...);		\
	}																			\
	if constexpr (!(std::is_same_v<std::decay_t<_Func2>, std::nullptr_t>))		\
		postCall(this, temp, std::forward<_Args>(args) ...);					\
};																				\
template <typename _Func1, typename _Func2, typename ..._Args>					\
void className::TransformTraverse(_Func1 prevCall, _Func2 postCall, _Args&&... args)		\
{																				\
	_TransformTraverse({}, prevCall, postCall, std::forward<_Args>(args) ...);	\
};


class SceneNode
{
public:
	struct Rendering
	{
		void* opaque = 0;
		ICamera* camera;
		IRenderableObject* renderableObject;
		size_t lightID;
	};

public:
	enum NODE_TYPE
	{
		RENDERABLE_NODE,
		CAMERA_NODE,
		LIGHT_NODE
	};

	static constexpr const char* NODE_TYPE_DESC[] = {
		"RENDERABLE_NODE",
		"CAMERA_NODE",
		"LIGHT_NODE"
	};

	//not refer as physics object is STATIC or DYNAMIC
	enum NODE_STATE_TYPE
	{
		STATIC,

		//data in node change frequently
		DYNAMIC
	};

	enum CONTROLLER_TYPE
	{
		SCRIPTING,
		BUILT_IN
	};

public:
	void* m_controller = 0;
	void* m_physicsObject = 0;
	Rendering m_renderingObject;

	void (*m_deleter)(SceneNode*) = 0;

	NODE_TYPE m_nodeType = RENDERABLE_NODE;
	CONTROLLER_TYPE m_controllerType = BUILT_IN;
	NODE_STATE_TYPE m_stateType = STATIC;

	int32_t m_stateChange = 1;

	SceneNodeBlob m_blob;

	SceneNode* m_parent = 0;
	std::vector<SceneNode*> m_childs;

	Mat4x4 m_transform;

public:
	inline ~SceneNode()
	{
		//std::cout << "~SceneNode()\n";
		std::cout << "\'" << SceneNode::NODE_TYPE_DESC[m_nodeType] << "\' deleted.\n";
		switch (m_nodeType)
		{
		case SceneNode::RENDERABLE_NODE:
		{
			//m_blob.FreeAs<Mat4x4>();
		}
		break;
		case SceneNode::CAMERA_NODE:
		{
			m_blob.FreeAs<SceneNodeBlob::CameraBuffer>();
		}
		break;
		case SceneNode::LIGHT_NODE:
		{
			m_blob.FreeAs<Light>();
		}
		break;
		default:
			break;
		}

		if (m_deleter) m_deleter(this);

		for (auto& child : m_childs)
		{
			delete child;
		}
		m_childs.clear();
	};

public:
	SCENE_NODE_DEFINE_TRAVERSE;

public:
	inline auto& RenderingObject() 
	{ 
		return m_renderingObject; 
	};

	template <typename _PhysicsObject>
	inline _PhysicsObject& PhysicsObject() 
	{ 
		return *(_PhysicsObject*)m_physicsObject; 
	};

	inline auto& PhysicsObject() 
	{ 
		return m_physicsObject; 
	};

	inline auto& Controller() 
	{ 
		return m_controller; 
	};

	inline auto& Type() 
	{ 
		return m_nodeType; 
	};

	inline auto IsStateChange()
	{
		return m_stateChange > 0;
	};

	inline auto& StateChange()
	{
		return m_stateChange;
	};

	inline auto& Blob() 
	{ 
		return m_blob; 
	};

	inline auto& Transform()
	{
		return m_transform;
	};
	
	inline auto& Deleter()
	{
		return m_deleter;
	};

};

using NodeId = size_t;
using SceneQueriedNodeId = NodeId;

static constexpr size_t NodeId_Invalid{ static_cast<size_t>(-1) };

class SceneQueriedNode
{
public:
	NodeId m_id = NodeId_Invalid;
	SceneQueryContext* m_owner = 0;

	SceneNode* m_sharedNode;

	//for multithreading
	//when do query, copy m_sharedNode->m_blob to this->m_blob
	SceneNodeBlob m_blob;

	NodeId m_parent = 0;
	std::vector<NodeId> m_childs;

	Mat4x4 m_transform;

public:
	SCENE_NODE_DEFINE_TRAVERSE;

public:
	inline void Free()
	{
		std::cout << "Free()\n";
		switch (m_sharedNode->Type())
		{
		case SceneNode::RENDERABLE_NODE:
			m_blob.FreeAs<Mat4x4>();
			break;
		case SceneNode::CAMERA_NODE:
			m_blob.FreeAs<SceneNodeBlob::CameraBuffer>();
			break;
		case SceneNode::LIGHT_NODE:
			m_blob.FreeAs<Light>();
			break;
		default:
			break;
		}
	};

	inline auto GetSceneNode() 
	{ 
		return m_sharedNode; 
	};

	inline auto& Blob() 
	{
		return m_blob; 
	};

	inline auto& Transform()
	{
		return m_transform;
	};

	inline auto CurrentContext()
	{
		return m_owner;
	};

	inline bool HasController()
	{
		return m_sharedNode->m_controller != 0;
	};
	
};

//per frame query
class SceneQueryContext
{
public:
	Scene* m_owner = 0;
	size_t m_id = 0;
	std::mutex m_mutex;

	//queried data memory point to these buffers
	//ReusableVector<Mat4x4> m_transformsBuf;
	ReusableVector<SceneNodeBlob::CameraBuffer> m_camerasBuf;
	ReusableVector<Light> m_lightsBuf;

	//per frame
	ReusableVector<SceneQueriedNode> m_reuseNodes;

	//need update back to scene node
	std::vector<NodeId> m_needUpdateNodes;

	std::vector<int32_t*> m_decreState;
	std::vector<int32_t*> m_increState;

public:
	inline void BeginFrame()
	{
		//m_transformsBuf.Clear();
		//m_mutex.lock();
		m_camerasBuf.Clear();
		m_lightsBuf.Clear();
		m_reuseNodes.Clear();
	};

	inline void EndFrame();

	inline NodeId NewLightNode(SceneNode& sceneNode)
	{
		auto id = m_reuseNodes.NextIndex();
		auto ret = &m_reuseNodes[id];
		ret->Blob().m_memId = m_lightsBuf.NextIndex();
		ret->Blob().m_mem = &m_lightsBuf;

		//memory copy
		ret->Blob().AsLight() = sceneNode.Blob().AsLight();

		return id;
	};

	inline NodeId NewRenderableObjectNode(SceneNode& sceneNode)
	{
		auto id = m_reuseNodes.NextIndex();
		auto ret = &m_reuseNodes[id];

		//use transform of node

		//ret->Blob().m_memId = m_transformsBuf.NextIndex();
		ret->Blob().m_memId = NodeId_Invalid;
		//ret->Blob().m_mem = &m_transformsBuf;

		return id;
	};

	inline NodeId NewCameraNode(SceneNode& sceneNode)
	{
		auto id = m_reuseNodes.NextIndex();
		auto ret = &m_reuseNodes[id];
		ret->Blob().m_memId = m_camerasBuf.NextIndex();
		ret->Blob().m_mem = &m_camerasBuf;

		//memory copy
		ret->Blob().AsCamera() = sceneNode.Blob().AsCamera();

		return id;
	};

	inline NodeId NewNode(SceneNode& sceneNode)
	{
		switch (sceneNode.Type())
		{
		case SceneNode::RENDERABLE_NODE:
			return NewRenderableObjectNode(sceneNode);
			break;
		case SceneNode::CAMERA_NODE:
			return NewCameraNode(sceneNode);
			break;
		case SceneNode::LIGHT_NODE:
			return NewLightNode(sceneNode);
			break;
		default:
			break;
		}
	};

	inline auto& Node(NodeId id)
	{
		return m_reuseNodes[id];
	}

	inline void Traverse(
		SceneNode* currentSceneNode,
		NodeId& parentQueriedNode,
		NodeId& currentQueriedNode)
	{
		currentQueriedNode = NewNode(*currentSceneNode);

		Node(currentQueriedNode).m_childs.resize(currentSceneNode->m_childs.size());
		Node(currentQueriedNode).m_transform = currentSceneNode->Transform();
		Node(currentQueriedNode).m_sharedNode = currentSceneNode;
		Node(currentQueriedNode).m_parent = parentQueriedNode;
		Node(currentQueriedNode).m_owner = this;
		Node(currentQueriedNode).m_id = currentQueriedNode;

		size_t i = 0;
		for (auto& child : currentSceneNode->m_childs)
		{
			Traverse(child, currentQueriedNode, Node(currentQueriedNode).m_childs[i]);
			i++;
		}
	};

	//recursive copy
	inline NodeId NewFromSceneNode(SceneNode& sceneNode)
	{
		SceneNode* root = &sceneNode;
		NodeId queriedRootId = NodeId_Invalid;
		NodeId temp = NodeId_Invalid;
		Traverse(root, temp, queriedRootId);
		return queriedRootId;
	};

public:
	//flush back data to scene
	//update later
	inline void FlushBackData(SceneQueriedNode* node)
	{
		m_needUpdateNodes.push_back(node->m_id);
	};

	inline void DecrementState(SceneQueriedNode* node)
	{
		m_decreState.push_back(&node->m_sharedNode->StateChange());
	};

	inline void IncrementState(SceneQueriedNode* node)
	{
		m_increState.push_back(&node->m_sharedNode->StateChange());
	};

};

class Scene
{
public:
	enum PHYSICS_ENGINE
	{
		BULLET
	};

	PHYSICS_ENGINE m_physicsEngine = PHYSICS_ENGINE::BULLET;
	std::vector<SceneQueryContext*> m_contexts;
	//std::mutex m_mutex;
	//copy memory from SceneQueriedNode -> SceneDataNode
	

	//ReusableVector<Mat4x4> m_transformsBuf;
	ReusableVector<SceneNodeBlob::CameraBuffer> m_camerasBuf;
	ReusableVector<Light> m_lightsBuf;

public:
	inline Scene(Engine* engine)
	{

	};

	inline virtual ~Scene() 
	{
		for (auto& ctx : m_contexts)
		{
			delete ctx;
		}
	};

public:
	//in 3D, Query2D work by projection object to Oxz
	virtual void Query2D(SceneQueryContext* context, Rect2D* area, std::vector<SceneQueriedNode*>& output) = 0;
	//aligned axis rect
	virtual void Query2D(SceneQueryContext* context, AARect2D* area, std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query2D(SceneQueryContext* context, Trapezoid* area, std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query2D(SceneQueryContext* context, Circle* area, std::vector<SceneQueriedNode*>& output) = 0;


	virtual void Query3D(SceneQueryContext* context, Box* bounding, std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3D(SceneQueryContext* context, AABB* bounding, std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3D(SceneQueryContext* context, Frustum* bounding, std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3D(SceneQueryContext* context, Sphere* bounding, std::vector<SceneQueriedNode*>& output) = 0;


	//immutable node live on memory until scene deleted
	//immutable node contains static object, kinematic object, light, ... (objects that don't change their position)
	virtual void Query3DImmutableNodes(SceneQueryContext* context, Box* bounding, 
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DImmutableNodes(SceneQueryContext* context, AABB* bounding, 
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DImmutableNodes(SceneQueryContext* context, Frustum* bounding, 
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DImmutableNodes(SceneQueryContext* context, Sphere* bounding, 
		std::vector<SceneQueriedNode*>& output) = 0;

	//scene can remove or add mutable node dynamically
	//mutable node count should be small (around 1000 - 2000)
	//mutable node contains dynamic object, partical object, ...
	virtual void Query3DMutableNodes(SceneQueryContext* context, Box* bounding,
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DMutableNodes(SceneQueryContext* context, AABB* bounding,
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DMutableNodes(SceneQueryContext* context, Frustum* bounding,
		std::vector<SceneQueriedNode*>& output) = 0;
	virtual void Query3DMutableNodes(SceneQueryContext* context, Sphere* bounding,
		std::vector<SceneQueriedNode*>& output) = 0;


	virtual void LoadFromFile(const std::string& path) = 0;


	inline SceneQueryContext* NewQueryContext()
	{
		auto ret = new SceneQueryContext();
		ret->m_id = m_contexts.size();
		ret->m_owner = this;
		m_contexts.push_back(ret);
		return ret;
	};

	inline void BeginQuery(SceneQueryContext* context)
	{
		context->m_mutex.lock();
	};
	
	inline void EndQuery(SceneQueryContext* context)
	{
		context->m_mutex.unlock();
	};

	inline void BeginUpdate(SceneQueryContext* context)
	{
		for (auto& ctx : m_contexts)
		{
			ctx->m_mutex.lock();
		}
	};

	inline void EndUpdate(SceneQueryContext* context)
	{
		for (auto& ctx : m_contexts)
		{
			ctx->m_mutex.unlock();
		}
	};

public:
	inline SceneNode* NewLightNode()
	{
		auto ret = new SceneNode();
		ret->Type() = SceneNode::LIGHT_NODE;
		ret->Blob().m_memId = m_lightsBuf.NextIndex();
		ret->Blob().m_mem = &m_lightsBuf;
		return ret;
	};

	inline SceneNode* NewRenderableObjectNode()
	{
		auto ret = new SceneNode();
		ret->Type() = SceneNode::RENDERABLE_NODE;
		ret->Blob().m_memId = NodeId_Invalid;
		//ret->Blob().m_memId = m_transformsBuf.NextIndex();
		//ret->Blob().m_mem = &m_transformsBuf;
		return ret;
	};

	inline SceneNode* NewCameraNode()
	{
		auto ret = new SceneNode();
		ret->Type() = SceneNode::CAMERA_NODE;
		ret->Blob().m_memId = m_camerasBuf.NextIndex();
		ret->Blob().m_mem = &m_camerasBuf;
		return ret;
	};

};

inline void SceneQueryContext::EndFrame()
{
	if (m_needUpdateNodes.size() == 0
		&& m_decreState.size() == 0
		&& m_increState.size() == 0)
	{
		//m_mutex.unlock();
		return;
	}

	m_owner->BeginUpdate(this);

	for (auto& node : m_needUpdateNodes)
	{
		auto sceneNode = Node(node).GetSceneNode();

		sceneNode->Transform() = Node(node).Transform();

		switch (sceneNode->Type())
		{
		/*case SceneNode::RENDERABLE_NODE:
			sceneNode->Transform() = Node(node).Blob().AsTransform();
			break;*/
		case SceneNode::CAMERA_NODE:
			sceneNode->Blob().AsCamera().proj = Node(node).Blob().AsCamera().proj;
			break;
		case SceneNode::LIGHT_NODE:
			sceneNode->Blob().AsLight() = Node(node).Blob().AsLight();
			break;
		default:
			break;
		}
		(sceneNode->StateChange())++;
	}
	m_needUpdateNodes.clear();

	for (auto& state : m_increState)
	{
		(*state)++;
	}
	m_increState.clear();

	for (auto& state : m_decreState)
	{
		(*state)--;
	}
	m_decreState.clear();

	m_owner->EndUpdate(this);
	//m_mutex.unlock();
};

SCENE_NODE_IMPL_TRAVERSE(SceneNode, child);
SCENE_NODE_IMPL_TRAVERSE(SceneQueriedNode, (&(m_owner->Node(child))));