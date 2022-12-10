#pragma once

#include "Core/TypeDef.h"
#include "Components/SubSystemComponent.h"

NAMESPACE_BEGIN

class Renderer : SubSystemComponent<Renderer>
{
public:
	inline Renderer() {};
	inline virtual ~Renderer() {};

};

NAMESPACE_END