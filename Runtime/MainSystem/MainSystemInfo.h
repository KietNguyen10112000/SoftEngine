#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class MainSystemInfo
{
public:
	constexpr static size_t COUNT = 8;

	constexpr static ID RENDERING_ID			= 0;
	constexpr static ID PHYSICS_ID				= 1;
	constexpr static ID SCRIPTING_ID			= 2;
	constexpr static ID AUDIO_ID				= 4;


	static constexpr const char* COMPONENT_NAME[COUNT] = {
		"RenderingComponent",
		"PhysicsComponent",
		"ScriptComponent",
		"AudioComponent"
	};
};

#define MAIN_SYSTEM_FRIEND_CLASSES()	\
friend class Runtime;					\
friend class Scene;						\
friend class MainSystem;				\
friend class RenderingSystem;			\
friend class PhysicsSystem;				\
friend class ScriptingSystem;			\
friend class MainComponentDB;			\
friend class AudioSystem;

NAMESPACE_END