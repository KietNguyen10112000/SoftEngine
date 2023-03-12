#pragma once

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class Script : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::SCRIPT_SUBSYSTEM_COMPONENT_ID;

protected:
	

public:
	virtual void OnComponentAddedToScene(GameObject* obj) override;

};

NAMESPACE_END