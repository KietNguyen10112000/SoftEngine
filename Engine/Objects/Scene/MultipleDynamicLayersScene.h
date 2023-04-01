#pragma once

#include "Scene.h"
#include "DynamicLayer.h"

NAMESPACE_BEGIN

struct MultipleDynamicLayersSceneQuerySession final : public SceneQuerySession
{
	// we can share session between DynamicLayer
	DynamicLayerQuerySession session;

	virtual void Clear()
	{
		session.Clear();
	}
};

class MultipleDynamicLayersScene : Traceable<MultipleDynamicLayersScene>, public Scene
{
protected:
	using Base = Scene;

	constexpr static size_t NUM_LAYERS = 4;

	DynamicLayer m_layers[NUM_LAYERS] = {};
	std::atomic<size_t> m_layerObjsCount[NUM_LAYERS] = {};
	std::atomic<ID> m_layerIDs[NUM_LAYERS] = {};

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

public:
	MultipleDynamicLayersScene(Engine* engine);
	~MultipleDynamicLayersScene();

public:
	// Inherited via Scene
	virtual void AddDynamicObject(GameObject* obj);

	virtual void RemoveDynamicObject(GameObject* obj);

	virtual void RefreshDynamicObject(GameObject* obj);

	virtual void ReConstruct();

	virtual void Synchronize();

public:
	virtual UniquePtr<SceneQuerySession> NewDynamicQuerySession() override;
	// query dynamic objects
	virtual void AABBDynamicQueryAABox(const AABox& aaBox, SceneQuerySession* session) override;
	virtual void AABBDynamicQueryFrustum(const Frustum& frustum, SceneQuerySession* session) override;

};

NAMESPACE_END