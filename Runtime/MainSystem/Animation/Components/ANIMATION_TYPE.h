#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

enum ANIMATION_TYPE
{
	// game object based hierarchy animation
	ANIMATION_TYPE_GAME_OBJECT,

	// hierarchy will be organized in an array 
	ANIMATION_TYPE_ARRAY,

	// same as ANIMATION_TYPE_GAME_OBJECT but self organized inside animation system
	ANIMATION_TYPE_SELF_HIERARCHY
};

NAMESPACE_END