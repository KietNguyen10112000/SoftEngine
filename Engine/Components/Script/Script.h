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

public:
	virtual void OnComponentAddedToScene() final override;

	// Inherited via SubSystemComponent
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnComponentRemovedFromScene() override;

	virtual void SetAsMain() override;

	virtual void SetAsExtra() override;

	virtual void ResolveConflict() override;

	virtual bool IsConflict() override;

public:
	virtual void OnStart() = 0;
	virtual void OnUpdate(float dt) = 0;

};

NAMESPACE_END