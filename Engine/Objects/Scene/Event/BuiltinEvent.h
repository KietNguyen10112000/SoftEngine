#pragma once
#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class BUILTIN_EVENT
{
public:
	enum Enum
	{
		SCENE_ADD_OBJECT,
		SCENE_REMOVE_OBJECT,

		COUNT,
	};
	
};

//enum BUILTIN_EVENT_SUIT
class BUILTIN_EVENT_SUIT
{
public:
	enum Enum
	{
		SCENE_EVENT = 0,
		OBJECT_EVENT = 0,

		COUNT
	};
};

NAMESPACE_END