#pragma once

#include "Core/TypeDef.h"
#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class PhysicsComponent : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::PHYSICS_SUBSYSTEM_COMPONENT_ID;

	enum TYPE
	{
		STATIC,
		DYNAMIC,
		KINEMATIC
	};

protected:
	const TYPE m_TYPE = STATIC;

public:
	inline PhysicsComponent(TYPE type) : m_TYPE(type) {};
	inline virtual ~PhysicsComponent() {};

public:
	inline auto Type() const
	{
		return m_TYPE;
	}

};

NAMESPACE_END