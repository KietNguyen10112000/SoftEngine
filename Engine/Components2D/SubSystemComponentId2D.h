#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class SubSystemComponentId2D
{
public:
	constexpr static size_t RENDERING_SUBSYSTEM_COMPONENT_ID	= 0;
	constexpr static size_t PHYSICS_SUBSYSTEM_COMPONENT_ID		= 1;
	constexpr static size_t SCRIPT_SUBSYSTEM_COMPONENT_ID		= 2;


	// list of component that process data then flush to main object
	constexpr static size_t PROCESS_DATA_COMPONENTS[] = {
		PHYSICS_SUBSYSTEM_COMPONENT_ID,
		SCRIPT_SUBSYSTEM_COMPONENT_ID
	};


	// use root object
	constexpr static size_t IS_USE_ROOT_OBJECTS[] = {
		false,
		true,
		true,
	};


	// list of component priority, smaller is higher
	// eg: 
	//		PRIORITY[SCRIPT_SUBSYSTEM_COMPONENT_ID] == 0
	//		PRIORITY[PHYSICS_SUBSYSTEM_COMPONENT_ID] == 1
	// the highest PRIORITY component will be the main component
	// PRIORITY of !PROCESS_DATA_COMPONENTS must be -1
	constexpr static size_t PRIORITY[] = {
		(size_t)(-1),
		0,
		1
	};
};

NAMESPACE_END