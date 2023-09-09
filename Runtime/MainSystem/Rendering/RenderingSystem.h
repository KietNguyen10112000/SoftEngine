#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Modules/Graphics/GraphicsFundamental.h"

#include "Common/ComponentQueryStructures/DoubleBVH.h"

NAMESPACE_BEGIN

class Camera;

class RenderingSystem : public MainSystem
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

};

NAMESPACE_END