#pragma once

#include "Scene.h"


//use bullet as physics backend
class SimpleScene : public Scene
{
public:
	std::vector<SceneNode*> m_nodes;
	//std::vector<NodeId> m_queryNodeIds;

public:
	SimpleScene(Engine* engine);
	~SimpleScene();

public:
	// Inherited via Scene
	virtual void Query2D(SceneQueryContext* context, Rect2D* area, std::vector<NodeId>& output) override;

	virtual void Query2D(SceneQueryContext* context, AARect2D* area, std::vector<NodeId>& output) override;

	virtual void Query2D(SceneQueryContext* context, Trapezoid* area, std::vector<NodeId>& output) override;

	virtual void Query2D(SceneQueryContext* context, Circle* area, std::vector<NodeId>& output) override;

	virtual void Query3D(SceneQueryContext* context, Box* bounding, std::vector<NodeId>& output) override;

	virtual void Query3D(SceneQueryContext* context, AABB* bounding, std::vector<NodeId>& output) override;

	virtual void Query3D(SceneQueryContext* context, Frustum* bounding, std::vector<NodeId>& output) override;

	virtual void Query3D(SceneQueryContext* context, Sphere* bounding, std::vector<NodeId>& output) override;

	virtual void LoadFromFile(const std::string& path) override;


	


	// Inherited via Scene
	virtual void Query3DImmutableNodes(SceneQueryContext* context, Math::Box* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DImmutableNodes(SceneQueryContext* context, Math::AABB* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DImmutableNodes(SceneQueryContext* context, Math::Frustum* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DImmutableNodes(SceneQueryContext* context, Math::Sphere* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DMutableNodes(SceneQueryContext* context, Math::Box* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DMutableNodes(SceneQueryContext* context, Math::AABB* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DMutableNodes(SceneQueryContext* context, Math::Frustum* bounding, std::vector<size_t, std::allocator<size_t>>& output);

	virtual void Query3DMutableNodes(SceneQueryContext* context, Math::Sphere* bounding, std::vector<size_t, std::allocator<size_t>>& output);


	// Inherited via Scene
	virtual void AddNodes(SceneNode** nodes, size_t count) override;

	virtual void RemoveNodes(SceneNode** nodes, size_t count) override;


	// Inherited via Scene
	virtual void Filter(std::vector<SceneNode*, std::allocator<SceneNode*>>& output, Scene::FilterFunction func = (Scene::FilterFunction)0);

};