#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Modules/Graphics/GraphicsFundamental.h"

#include "Common/ComponentQueryStructures/DoubleBVH.h"
#include "Common/Base/AsyncServer.h"

NAMESPACE_BEGIN

class Camera;

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
	friend class Camera;

	struct DisplayingCamera
	{
		Camera* camera;
		GRAPHICS_VIEWPORT viewport;
	};

	DoubleBVH m_bvh;

	// camera that is displaying to screen
	std::vector<DisplayingCamera> m_displayingCamera;

	// camera that scene will be rendered to (that maybe not displayed to screen but still visiable somewhere)
	std::vector<Camera*> m_activeCamera;

	SharedPtr<GraphicsPipeline>				m_testPipeline;
	SharedPtr<GraphicsVertexBuffer>			m_testVertexBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_testConstantBuffer;

public:
	RenderingSystem(Scene* scene);
	~RenderingSystem();

private:
	void AddCamera(Camera* camera);
	void RemoveCamera(Camera* camera);

public:
	void DisplayCamera(Camera* camera, const GRAPHICS_VIEWPORT& viewport);
	void HideCamera(Camera* camera);


	// Inherited via MainSystem
	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void Iteration(float dt) override;

	virtual void PrevIteration() override;

	virtual void PostIteration() override;

};

NAMESPACE_END