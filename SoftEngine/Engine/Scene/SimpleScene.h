#pragma once

#include "Scene.h"


class SimpleScene : public Scene
{
public:
	std::vector<SceneObject*> m_objects;
	std::vector<SharedObject> m_sharedObjects;

public:
	SimpleScene(Engine* engine);
	~SimpleScene();


	// Inherited via Scene
	virtual void Query3DObjects(SceneQueryContext* context, Box* bounding, SceneObjectList& output) override;


	virtual void Query3DObjects(SceneQueryContext* context, AABB* bounding, SceneObjectList& output) override;


	virtual void Query3DObjects(SceneQueryContext* context, Frustum* bounding, SceneObjectList& output) override;


	virtual void Query3DObjects(SceneQueryContext* context, Sphere* bounding, SceneObjectList& output) override;


	virtual void Query3DSharedObjects(SceneQueryContext* context, Box* bounding, SceneSharedObjectList& output) override;


	virtual void Query3DSharedObjects(SceneQueryContext* context, AABB* bounding, SceneSharedObjectList& output) override;


	virtual void Query3DSharedObjects(SceneQueryContext* context, Frustum* bounding, SceneSharedObjectList& output) override;


	virtual void Query3DSharedObjects(SceneQueryContext* context, Sphere* bounding, SceneSharedObjectList& output) override;


	virtual void AddSharedObjects(SharedObject* objects, size_t count) override;


	virtual void RemoveSharedObjects(SharedObject* objects, size_t count) override;


	virtual void AddObjects(SceneObject* nodes, size_t count) override;


	virtual void RemoveObjects(SceneObject* nodes, size_t count) override;


	virtual void FilterObjects(SceneObjectList& output, FilterFunction func = 0) override;


	virtual void FilterSharedObjects(SceneSharedObjectList& output, FilterFunction func = 0) override;


};