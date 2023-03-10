#pragma once

#include "Scene.h"
#include "DynamicLayer.h"

NAMESPACE_BEGIN

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
	MultipleDynamicLayersScene();
	~MultipleDynamicLayersScene();

public:
	// Inherited via Scene
	virtual void AddDynamicObject(GameObject* obj);

	virtual void RemoveDynamicObject(GameObject* obj);

	virtual void RefreshDynamicObject(GameObject* obj);

	virtual void ReConstruct();

	virtual void Synchronize();

};

NAMESPACE_END