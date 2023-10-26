#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class MODIFICATION_STATE
{
public:
	enum STATE
	{
		NONE,
		ADDING,
		REMOVING,
		CROSS_SCENE
	};

};

NAMESPACE_END