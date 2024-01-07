#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

enum ANIMATION_TYPE
{
	// game object based hierarchy animation
	ANIMATION_TYPE_SKELETAL_GAME_OBJECT,

	// hierarchy will be organized in an array 
	ANIMATION_TYPE_SKELETAL_ARRAY,

	// same as ANIMATION_TYPE_GAME_OBJECT but self organized inside animation system
	ANIMATION_TYPE_SKELETAL_SELF_HIERARCHY,

	ANIMATION_TYPE_NONE
};

NAMESPACE_END