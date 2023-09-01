#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class RUNTIME_EVENT
{
public:
	enum
	{
		//======================================
		// event args = { engine, scene }, argc == 2
		SCENE_ON_INITIALIZE,
		SCENE_ON_SETUP,
		SCENE_ON_START,
		//======================================

		COUNT,
	};
};

NAMESPACE_END