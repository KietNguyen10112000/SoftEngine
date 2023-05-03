#pragma once

#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

#include "Objects2D/Scene2D/Scene2D.h"

#include "Objects2D/GameObject2D.h"
#include "Objects/Async/AsyncTaskRunner.h"

NAMESPACE_BEGIN

class API Script2D : Traceable<Script2D>, public SubSystemComponent2D, public AsyncTaskRunner
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId2D::SCRIPT_SUBSYSTEM_COMPONENT_ID;


	friend class ScriptSystem2D;
	friend class GameObject2D;
	friend class Scene2D;

private:
	ID m_onUpdateId		= INVALID_ID;
	ID m_onGUIId		= INVALID_ID;
	ID m_onCollideId	= INVALID_ID;

protected:
	Scene2D* m_scene = nullptr;

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

public:
	virtual void OnStart() {};
	virtual void OnUpdate(float dt) {};
	virtual void OnGUI() {};

public:
	inline auto Input()
	{
		return m_scene->GetInput();
	}

	inline auto& Position()
	{
		return m_object->Position();
	}

	inline auto& Rotation()
	{
		return m_object->Rotation();
	}

	inline auto& Scale()
	{
		return m_object->Scale();
	}

};

NAMESPACE_END