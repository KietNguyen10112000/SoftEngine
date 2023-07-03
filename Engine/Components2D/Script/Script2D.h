#pragma once

#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

#include "Objects2D/Scene2D/Scene2D.h"

#include "Objects2D/GameObject2D.h"
#include "Objects/Async/AsyncTaskRunner.h"
#include "Objects2D/Physics/Collision/Collision2DPair.h"

#include "PhysicsInterface.h"

#include "Script2DMeta.h"

#include <bitset>

NAMESPACE_BEGIN

#define SCRIPT2D_DEFAULT_METHOD(clazz)													\
TRACEABLE_FRIEND();																		\
virtual void InitializeClassMetaData() override {_InitializeClassMetaData<clazz>();}

class API Script2D : Traceable<Script2D>, public SubSystemComponent2D, public AsyncTaskRunner
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId2D::SCRIPT_SUBSYSTEM_COMPONENT_ID;


	friend class ScriptSystem2D;
	friend class GameObject2D;
	friend class Scene2D;

private:
	enum PROCESS_FLAG
	{
		ON_COLLISION_ENTER,
		ON_COLLIDE,
		ON_COLLISION_EXIT
	};

	std::bitset<64> m_overriddenVtbIdx = {};

	ID m_onGUIId				= INVALID_ID;

	ID m_onCollideId			= INVALID_ID;

protected:
	Scene2D* m_scene = nullptr;

	PhysicsInterface m_physicsInterface;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		AsyncTaskRunner::Trace(tracer);
	}

private:
	virtual void OnComponentAddedToScene() final override;

	// Inherited via SubSystemComponent
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnComponentRemovedFromScene() override;

	virtual void OnObjectRefresh() override {};

	virtual void SetAsMain() override;

	virtual void SetAsExtra() override;

	virtual void ResolveBranch() override;

	virtual bool IsNewBranch() override;

	inline void BeginUpdate()
	{
		AsyncTaskRunner::Flush();
	}

	inline void EndUpdate()
	{
	}

	inline void Update(float dt)
	{
		BeginUpdate();
		OnUpdate(dt);
		EndUpdate();
	}

protected:
#define SET_VTB_OVERRIDDEN_IDX(funcName, idxName)								\
{																				\
	auto f = &Script2D::funcName;												\
	auto p = (void*&)f;															\
	auto f1 = &ChildClass::funcName;											\
	auto p1 = (void*&)f1;														\
	if (p1 != p)																\
	{																			\
		m_overriddenVtbIdx.set(idxName);										\
	}																			\
}


	template <typename ChildClass>
	inline void _InitializeClassMetaData()
	{
		SET_VTB_OVERRIDDEN_IDX(OnGUI,				Script2DMeta::Get().onGUIVtbIdx);
		SET_VTB_OVERRIDDEN_IDX(OnCollide,			Script2DMeta::Get().onCollideVtbIdx);
		SET_VTB_OVERRIDDEN_IDX(OnCollisionEnter,	Script2DMeta::Get().onCollisionEnterVtbIdx);
		SET_VTB_OVERRIDDEN_IDX(OnCollisionExit,		Script2DMeta::Get().onCollisionExitVtbIdx);
		SET_VTB_OVERRIDDEN_IDX(OnUpdate,			Script2DMeta::Get().onUpdateVtbIdx);
	}

#undef SET_VTB_OVERRIDDEN_IDX

	virtual void InitializeClassMetaData() = 0;

public:
	// methods for user override
	virtual void OnStart();
	virtual void OnUpdate(float dt);
	virtual void OnCollide(GameObject2D* another, const Collision2DPair& pair);
	virtual void OnCollisionEnter(GameObject2D* another, const Collision2DPair& pair);
	virtual void OnCollisionExit(GameObject2D* another, const Collision2DPair& pair);
	virtual void OnGUI();

public:
	inline auto Input()
	{
		return m_scene->GetInput();
	}

	inline auto* Physics()
	{
		return &m_physicsInterface;
	}

	inline auto& Position()
	{
		return m_object->Transform().Translation();
	}

	inline auto& Rotation()
	{
		return m_object->Transform().Rotation();
	}

	inline auto& Scale()
	{
		return m_object->Transform().Scale();
	}

	inline Scene2D* GetScene()
	{
		return m_scene;
	}

};

NAMESPACE_END