#pragma once

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

#include "Objects/Scene/Scene.h"

#include "Objects/GameObject.h"

NAMESPACE_BEGIN

class Script : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::SCRIPT_SUBSYSTEM_COMPONENT_ID;

protected:
	friend class ScriptSystem;

	ID m_scriptId = INVALID_ID;
	Scene* m_scene = nullptr;

	Transform* m_transform;

private:
	virtual void OnComponentAddedToScene() final override;

	// Inherited via SubSystemComponent
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnComponentRemovedFromScene() override;

	virtual void SetAsMain() override;

	virtual void SetAsExtra() override;

	virtual void ResolveConflict() override;

	virtual bool IsConflict() override;

	inline void BeginUpdate()
	{
		auto& transform = m_object->m_transform;
		m_transform = transform.GetWriteHead();
		auto read = transform.GetReadHead();
		*m_transform = *read;
	}

	inline void EndUpdate()
	{
		auto& transform = m_object->m_transform;
		transform.UpdateReadWriteHead();
	}

	inline void Update(float dt)
	{
		BeginUpdate();
		OnUpdate(dt);
		EndUpdate();
	}

public:
	virtual void OnStart() = 0;
	virtual void OnUpdate(float dt) = 0;

public:
	inline auto Input()
	{
		return m_scene->GetInput();
	}

	inline auto& Transform()
	{
		return *m_transform;
	}

};

NAMESPACE_END