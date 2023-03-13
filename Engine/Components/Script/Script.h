#pragma once

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class Script : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::SCRIPT_SUBSYSTEM_COMPONENT_ID;

protected:
	friend class ScriptSystem;

	ID m_scriptId = INVALID_ID;

	GameObject* m_object = nullptr;

public:
	virtual void OnComponentAddedToScene(GameObject* obj) final override;

	// Inherited via SubSystemComponent
	virtual void OnComponentAdded(GameObject* object) override;

	virtual void OnComponentRemoved(GameObject* object) override;

	virtual void OnComponentRemovedFromScene(GameObject* object) override;

	virtual void SetAsMain(GameObject* object) override;

	virtual void SetAsExtra(GameObject* object) override;

	virtual void ResolveConflict(GameObject* object) override;

	virtual bool IsConflict() override;

public:
	virtual void OnStart() = 0;
	virtual void OnUpdate(float dt) = 0;

};

NAMESPACE_END