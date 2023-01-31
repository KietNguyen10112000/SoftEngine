#pragma once

#include "Core/TypeDef.h"
#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class PhysicsComponent : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::PHYSICS_SUBSYSTEM_COMPONENT_ID;

	inline PhysicsComponent() {};
	inline virtual ~PhysicsComponent() {};

};

NAMESPACE_END