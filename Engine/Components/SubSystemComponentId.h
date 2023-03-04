#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class SubSystemComponentId
{
public:
	constexpr static size_t RENDERING_SUBSYSTEM_COMPONENT_ID	= 0;
	constexpr static size_t PHYSICS_SUBSYSTEM_COMPONENT_ID		= 1;
	constexpr static size_t SCRIPT_SUBSYSTEM_COMPONENT_ID		= 2;

};

NAMESPACE_END