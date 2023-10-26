#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "TaskSystem/TaskSystem.h"

#include "Modules/Graphics/GraphicsFundamental.h"

#include "Common/ComponentQueryStructures/DoubleBVH.h"
#include "Common/Base/AsyncServer.h"

#include "Resources/Texture2D.h"

#include "CAMERA_PRIORITY.h"

#include "BuiltinConstantBuffers.h"

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

	struct CollectInputForCameraParams
	{
		RenderingSystem* renderingSystem;
		BaseCamera* camera;
		AABBQuerySession* querySession;
	};

	DoubleBVH m_bvh;

	// camera that is displaying to screen
	std::vector<DisplayingCamera> m_displayingCamera;

	// camera that scene will be rendered to (that maybe not displayed to screen but still visiable somewhere)
	std::vector<BaseCamera*> m_activeCamera[CAMERA_PRIORITY::CAMERA_PRIORITY_COUNT];

	std::vector<BaseCamera*>					m_cameras;
	std::vector<Task>							m_collectInputForCameraTasks;
	std::vector<CollectInputForCameraParams>	m_collectInputForCameraParams;
	std::vector<AABBQuerySession*>				m_collectInputForCameraRets;

	GRAPHICS_VIEWPORT m_defaultViewport = {};

	BuiltinConstantBuffers* m_builtinConstantBuffers = BuiltinConstantBuffers::Get();

	BuiltinConstantBuffers::CameraData m_cameraData;

public:
	RenderingSystem(Scene* scene);
	~RenderingSystem();

private:
	void AddCamera(BaseCamera* camera, CAMERA_PRIORITY priority);
	void RemoveCamera(BaseCamera* camera);

	void CollectInputForEachCamera();
	void SetBuiltinConstantBufferForCamera(BaseCamera* camera);
	void RenderForEachCamera();
	void DisplayAllCamera();

public:
	void DisplayCamera(BaseCamera* camera, const GRAPHICS_VIEWPORT& viewport);
	void HideCamera(BaseCamera* camera);


	// Inherited via MainSystem
	virtual void BeginModification() override;

	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void EndModification() override;

	virtual void Iteration(float dt) override;

	virtual void PrevIteration() override;

	virtual void PostIteration() override;

public:
	inline GRAPHICS_VIEWPORT GetDefaultViewport()
	{
		return m_defaultViewport;
	}

	inline auto* GetBuiltinConstantBuffers()
	{
		return m_builtinConstantBuffers;
	}
};

NAMESPACE_END