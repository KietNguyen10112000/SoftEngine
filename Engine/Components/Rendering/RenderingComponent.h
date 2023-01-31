#pragma once

#include "Core/TypeDef.h"
#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class RenderingComponent : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::RENDERING_SUBSYSTEM_COMPONENT_ID;

	inline RenderingComponent() {};
	inline virtual ~RenderingComponent() {};

};

NAMESPACE_END