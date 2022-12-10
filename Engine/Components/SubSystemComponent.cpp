#include "SubSystemComponent.h"

#include "Renderers/Renderer.h"

NAMESPACE_BEGIN

constexpr size_t RENDERER_SUBSYSTEM_COMPONENT_ID = 0;

size_t SubSystemComponent<Renderer>::s_id = RENDERER_SUBSYSTEM_COMPONENT_ID;


NAMESPACE_END