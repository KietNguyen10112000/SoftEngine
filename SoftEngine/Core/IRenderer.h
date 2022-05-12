#pragma once

#include <string>

#include <ILightSystem.h>

class Object;
class ICamera;
class IRenderableObject;

class RenderPipeline;
class VertexBuffer;
class IndexBuffer;
class LightSystem;

class VertexShader;
class PixelShader;

class HullShader;
class GeometryShader;
class DomainShader;

enum PRIMITIVE_TOPOLOGY;

//procotype
//enum PRIMITIVE_TOPOLOGY
//{
//	UNDEFINED,
//	POINT_LIST,
//	LINE_LIST,
//	LINE_STRIP,
//	TRIANGLE_LIST,
//	TRIANGLE_STRIP
//};

struct ForwardShadingType
{
	int type;
	Vec3 padding;

	enum TYPE
	{
		PBR_SHADING = 0,
		USE_NORMAL_MAP_AND_MATERIAL = 1, //use normal map + material
		USE_MATERIAL_MAP = 2, //use texture2d as material
	};
};

struct ForwardShadingMaterial
{
	Vec3 ambient = {};
	float specular = 1;
	Vec3 diffuse = {};
	float shininess = 0;
};


class IRenderer
{
public:
	virtual ~IRenderer() {};

public:
	virtual void CopyGlobal(IRenderer* renderer) = 0;
	virtual std::wstring ShaderDirectory() = 0;

public:
	//present to camera setted by SetTargetCamera()
	virtual void Present() = 0;

	//renderer will render to this camera
	virtual void SetTargetCamera(ICamera* camera) = 0;

	virtual ICamera* GetTargetCamera() = 0;

	virtual void ClearFrame(float color[4]) = 0;

public:
	inline virtual void SetPrimitiveTopology(PRIMITIVE_TOPOLOGY topology) = 0;

public:
	virtual void SetRenderPipeline(RenderPipeline* rpl) = 0;
	virtual RenderPipeline*& DefaultRenderPipeline() = 0;
	virtual RenderPipeline*& CurrentRenderPipeline() = 0;

	virtual void Render(Object* obj) = 0;

	virtual void Render(VertexBuffer* vb) = 0;
	virtual void Render(VertexBuffer* vb, IndexBuffer* ib) = 0;
	virtual void Render(RenderPipeline* rpl, VertexBuffer* vb) = 0;
	virtual void Render(RenderPipeline* rpl, VertexBuffer* vb, IndexBuffer* ib) = 0;


	virtual void RenderInstance(RenderPipeline* rpl, 
		VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib) = 0;
	virtual void RenderInstance(RenderPipeline* rpl, 
		VertexBuffer* vb, VertexBuffer* instanceBuffer, uint32_t instanceCount) = 0;

public:
	virtual void RenderShadow(VertexBuffer* vb, IndexBuffer* ib) = 0;
	virtual void RenderInstanceShadow(VertexBuffer* vb, 
		VertexBuffer* instanceBuffer, uint32_t instanceCount, IndexBuffer* ib) = 0;

	//use to mix renderpipeline
	//redirect render call to func
	//func procotype: void (renderer, target, rpl, vb, ib)
	//renderer	: current renderer
	//target	: locked rpl
	//rpl		: render call rpl
	//vb		: vertex buffer
	//ib		: index buffer
	virtual void RedirectRenderPipeline(RenderPipeline* target, 
		void (*func)(
			IRenderer*, 
			RenderPipeline*, 
			RenderPipeline*, 
			VertexBuffer**, IndexBuffer**, uint32_t*)) = 0;
	virtual void RestoreRenderPipeline() = 0;

	virtual void NativeSetShaders(VertexShader*, HullShader*, DomainShader*, GeometryShader*, PixelShader*) = 0;

public:
	virtual void BeginTransparency() = 0;
	virtual void EndTransparency() = 0;

	// arg == 1 => use depthh buffer
	virtual void BeginUI(int arg = 0) = 0;
	virtual void EndUI() = 0;

	virtual void PresentLastFrame() = 0;

public:
	//optional
	inline virtual void VisualizeBackgroundRenderPipeline(int arg = 0) {};

	//optional
	//some engine not use light system like 2D engine, ...
	inline virtual class LightSystem* LightSystem() { return nullptr; };
	inline virtual class PostProcessor* PostProcessor() { return nullptr; };
	inline virtual void AttachLightSystem(class LightSystem* lightSystem) {};

	//optional
	//renderer will render skybox as background if it attached
	inline virtual void AttachSkyBox(IRenderableObject* skybox) {};

public:
	virtual float GetRenderWidth() = 0;
	virtual float GetRenderHeight() = 0;

};