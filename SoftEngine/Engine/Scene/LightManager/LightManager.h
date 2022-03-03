#pragma once

#include <IRenderer.h>

//do lights culling, shadow culling, cascade shadow, ...
//lights visualize, ...
class LightManager
{
public:
	//not own
	IRenderer* m_renderer = 0;
	LightSystem* m_lightSystem = 0;

	struct LightRenderInfo
	{
		LightID id;
		float coeff;
		Vec2 padding;
	};

	struct LightRenderObj
	{
		//not own
		IRenderableObject* obj = 0;
		Vec3* position = 0;

		LightRenderInfo info;
	};

	//can replace with quad tree
	std::vector<LightRenderObj> m_list;

	RenderPipeline* m_rpl = 0;

public:
	LightManager(IRenderer* renderer);

public:
	//add obj to render at light position
	void AddLightRenderObject(LightID id, IRenderableObject*& obj, const LightRenderInfo& material = { 0, 0.9f,{} });

	void Render(IRenderer* renderer);
public:
	inline auto LightSystem() { return m_lightSystem; };

};