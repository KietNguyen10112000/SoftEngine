#pragma once
#include "Core/TypeDef.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class GraphicsCommandList
{
public:
	virtual ~GraphicsCommandList() {};

public:
	virtual void ClearScreen(const Vec4& color) = 0;

};

NAMESPACE_END