#pragma once

#include "SceneConfig.h"

#include "Core/Templates/Pointers.h"
#include "Core/Variant/Memory.h"

#include "Math/Math.h"
#include "Math/AABB.h"

#include "SceneQueryContext.h"

//#define UNROLL

using SceneObjectExternalData = Memory;

class Controller;
class PhysicsObject;

class IObject;
class IRenderableObject;
class ICamera;

#define  SCENE_NODE_DEFINE_TRAVERSE												\
template <typename _Func1, typename _Func2, typename ..._Args> 					\
void _TransformTraverse(const Mat4x4& globalTransform, 							\
	_Func1 prevCall, _Func2 postCall, _Args&&... args);							\
template <typename _Func1, typename _Func2, typename ..._Args>					\
void TransformTraverse(_Func1 prevCall, _Func2 postCall, _Args&&... args);


#define SCENE_NODE_IMPL_TRAVERSE(className, _child)								\
template <typename _Func1, typename _Func2, typename ..._Args> 					\
void className::_TransformTraverse(const Mat4x4& globalTransform, 				\
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


/// 
/// don't use virtual
/// just for reuse data member
/// 
class SceneObjectBase
{
public:
	union Rendering
	{
		void* opaque = 0;
		IObject* object;
		ICamera* camera;
		IRenderableObject* renderableObject;
		size_t lightID;
	};

	struct LocalStorage
	{
		bool m_isReading = false;
	};

	enum TYPE
	{
		RENDERABLE_OBJECT,
		CAMERA_OBJECT,
		LIGHT_OBJECT
	};

	static constexpr const char* TYPE_DESC[] = {
		"RENDERABLE_OBJECT",
		"CAMERA_OBJECT",
		"LIGHT_OBJECT"
	};

public:
	///
	/// m_isReading[i] == 1 => thread i (or query ctx i) is reading this Node
	/// when this node avaiabale, thread can write data to this node
	/// ow, thread just read data from this node only
	/// 
	bool m_isReading[NUM_QUERY_CONTEXT] = {};

	/// 
	/// for derived scene handling
	/// 
	void* m_sceneHandler = 0;

	Controller* m_controller = 0;
	PhysicsObject* m_physicsObject = 0;
	Rendering m_renderingObject;

	TYPE m_type = RENDERABLE_OBJECT;

	///
	/// shared resouce, read and write simultaneously
	/// if data changed, rendering thread will update data to rendering object and set m_isDataChanged = false
	/// when logic thread make data changed, it also set m_isDataChanged = true
	/// 
	/// no data-race here because
	/// 
	/// rendering thread => rt
	/// logic thread => lt
	/// 
	/// case 1:
	///		lt set m_isDataChanged = true
	///		then
	///		rt see m_isDataChanged = true, set m_isDataChanged = false
	///		=> valid logic
	/// 
	/// case 2:
	///		rt see m_isDataChanged = true, set m_isDataChanged = false
	///		then
	///		lt set m_isDataChanged = true
	///		=> m_isDataChanged = true 
	///		=> in the next update, rt still see m_isDataChanged = true and then set m_isDataChanged = false
	///		=> still valid logic
	/// 
	/// case 3:
	///		rt see m_isDataChanged = false
	///		then
	///		lt set m_isDataChanged = true
	///		=> rt will not update data to rendering object
	///		=> but in next update, rt still see m_isDataChanged = true and then set m_isDataChanged = false
	///		=> still valid logic
	/// 
	/// case 4
	///		rt see m_isDataChanged = true
	///		then
	///		lt set m_isDataChanged = true
	///		=> m_isDataChanged == true
	///		then
	///		rt set m_isDataChanged = false
	///		=> still valid logic
	///		
	///
	bool m_isDataChanged = true;
	bool m_isExternalDataChanged = true;


	//=======================data=============================
	// critical resource must read and write sequently
	Mat4x4 m_transform;
	AABB m_aabb;
	// extenal data
	SceneObjectExternalData m_externalData;
	//========================================================
public:
	~SceneObjectBase();

public:
	inline bool IsAvailableToWrite()
	{
		bool done = true;

		// will compiler unroll for me ? :))
		for (size_t i = 0; i < NUM_QUERY_CONTEXT; i++)
		{
			done &= (m_isReading[i] == 0);
		}

		return done;
	};

	inline void AcquireReading(SceneQueryContext* context)
	{
		m_isReading[context->GetID()] = true;
	};

	inline void ReleaseReading(SceneQueryContext* context)
	{
		m_isReading[context->GetID()] = false;
	};

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
		return m_type;
	};

	inline auto& Transform()
	{
		return m_transform;
	};

	inline auto& ExternalData()
	{
		return m_externalData;
	};

	inline auto& DataChanged()
	{
		return m_isDataChanged;
	};

	inline auto& ExternalDataChanged()
	{
		return m_isExternalDataChanged;
	};
	
};

/// 
/// Scene object
/// 
/// This object will be deleted by scene
/// 
class SceneObject : public SceneObjectBase
{
public:
	using Base = SceneObjectBase;

public:
	void (*m_deleter)(SceneObject*) = 0;

	SceneObject* m_parent;
	std::vector<SceneObject*> m_childs;

public:
	inline ~SceneObject()
	{
		if (m_deleter) m_deleter(this);
		
		for (auto& child : m_childs)
		{
			delete child;
		}
	};

public:
	SCENE_NODE_DEFINE_TRAVERSE;

	inline auto& Deleter()
	{
		return m_deleter;
	};
};


/// 
/// Scene Shared object
/// 
/// This object can be add, remove from scene dynamically
/// Also exist when scene is not created
/// 
/// using SharedPtr<SceneSharedObject>
/// 
class SceneSharedObject : public SceneObjectBase
{
public:
	using Base = SceneObjectBase;

public:
	void (*m_deleter)(SceneSharedObject*) = 0;

	WeakPtr<SceneSharedObject> m_parent;
	std::vector<SharedPtr<SceneSharedObject>> m_childs;

	
public:
	inline ~SceneSharedObject()
	{
		if (m_deleter) m_deleter(this);
		m_childs.clear();
	};

public:
	SCENE_NODE_DEFINE_TRAVERSE;

	inline auto& Deleter()
	{
		return m_deleter;
	};

};


SCENE_NODE_IMPL_TRAVERSE(SceneObject, child);
SCENE_NODE_IMPL_TRAVERSE(SceneSharedObject, child);


#undef SCENE_NODE_DEFINE_TRAVERSE
#undef SCENE_NODE_IMPL_TRAVERSE