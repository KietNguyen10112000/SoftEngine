#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Modules/Graphics/GraphicsFundamental.h"

#include "Common/ComponentQueryStructures/DoubleBVH.h"
#include "Common/Base/AsyncServer.h"

#include "Resources/Texture2D.h"

#include "CAMERA_PRIORITY.h"

NAMESPACE_BEGIN

class BaseCamera;

class RenderingSystemCommand
{
public:
	enum CmdID
	{
		CMD0,
		COUNT
	};
};

class RenderingSystem : public MainSystem, public AsyncServer2<RenderingSystem, RenderingSystemCommand::COUNT>
{
private:
	friend class BaseCamera;
	friend class Camera;

	struct DisplayingCamera
	{
		BaseCamera* camera;
		GRAPHICS_VIEWPORT viewport;
	};

	DoubleBVH m_bvh;

	// camera that is displaying to screen
	std::vector<DisplayingCamera> m_displayingCamera;

	// camera that scene will be rendered to (that maybe not displayed to screen but still visiable somewhere)
	std::vector<BaseCamera*> m_activeCamera[CAMERA_PRIORITY::CAMERA_PRIORITY_COUNT];

	SharedPtr<GraphicsPipeline>				m_testPipeline;
	SharedPtr<GraphicsVertexBuffer>			m_testVertexBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_testCameraConstantBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_testObjectConstantBuffer;
	Resource<Texture2D>						m_testTexture2D;

	GRAPHICS_VIEWPORT m_defaultViewport = {};

public:
	RenderingSystem(Scene* scene);
	~RenderingSystem();

private:
	void AddCamera(BaseCamera* camera, CAMERA_PRIORITY priority);
	void RemoveCamera(BaseCamera* camera);

public:
	void DisplayCamera(BaseCamera* camera, const GRAPHICS_VIEWPORT& viewport);
	void HideCamera(BaseCamera* camera);


	// Inherited via MainSystem
	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void Iteration(float dt) override;

	virtual void PrevIteration() override;

	virtual void PostIteration() override;

public:
	inline GRAPHICS_VIEWPORT GetDefaultViewport()
	{
		return m_defaultViewport;
	}

};

NAMESPACE_END