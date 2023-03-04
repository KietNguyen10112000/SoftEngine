#pragma once

#include "Core/TypeDef.h"
#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

NAMESPACE_BEGIN

class Rendering : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::RENDERING_SUBSYSTEM_COMPONENT_ID;

	inline Rendering() {};
	inline virtual ~Rendering() {};

};

NAMESPACE_END