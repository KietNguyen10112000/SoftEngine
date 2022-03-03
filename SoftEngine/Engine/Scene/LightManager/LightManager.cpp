#include "LightManager.h"

#include <IObject.h>

LightManager::LightManager(IRenderer* renderer)
{
	m_lightSystem = renderer->LightSystem();
	m_renderer = renderer;

	m_rpl = RenderPipelineManager::Get(
		R"(struct VS_INPUT
        {
            vec3 pos; POSITION, PER_VERTEX #
            vec2 textCoord; TEXTCOORD, PER_VERTEX #
        };)",
		L"VSVisualize",
		L"Shadow/PSVisualize"
	)
}

void LightManager::AddLightRenderObject(LightID id, IRenderableObject*& obj, const LightRenderInfo& info)
{
	LightRenderObj robj;
	robj.obj = obj;
	robj.position = &m_lightSystem->GetLight(id).pos;
	robj.info = info;
	
	m_list.push_back(robj);
}

void LightManager::Render(IRenderer* renderer)
{
	renderer->RedirectRenderPipeline()
	for (auto& robj : m_list)
	{
		robj.obj->Render(renderer);
	}
}
