#pragma once

#include "Core/TypeDef.h"
#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

NAMESPACE_BEGIN

class RenderingSystem2D;

class Rendering2D : public SubSystemComponent2D
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId2D::RENDERING_SUBSYSTEM_COMPONENT_ID;

	inline Rendering2D() {};
	inline virtual ~Rendering2D() {};

public:
	// write data for render
	// and call render call
	virtual void Render(RenderingSystem2D* rdr) = 0;

};

NAMESPACE_END