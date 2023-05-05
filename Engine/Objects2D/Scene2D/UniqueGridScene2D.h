#pragma once

#include "Scene2D.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class UniqueGridScene2D : Traceable<UniqueGridScene2D>, public Scene2D
{
protected:
	using Base = Scene2D;
	
	constexpr static size_t STATIC_OUTSIDE		= -1;
	constexpr static size_t STATIC_INSIDE		= -2;
	constexpr static size_t DYNAMIC_OUTSIDE		= -3;
	constexpr static size_t DYNAMIC_INSIDE		= -4;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

public:
	struct Cell
	{
		size_t lastClearIteration = 0;
		std::Vector<GameObject2D*> staticObjects;
		std::Vector<GameObject2D*> dynamicObjects;
	};

	struct StaticObjectCells
	{
		StaticObjectCells* prev;
		StaticObjectCells* next;
	};

	Vec2 m_cellDimensions = {};
	size_t m_width;
	size_t m_height;
	std::Vector<Cell> m_cells;

	std::Vector<GameObject2D*> m_outsideStaticObjects;
	std::Vector<GameObject2D*> m_outsideDynamicObjects;

	std::Vector<GameObject2D*> m_dynamicObjects;

	StaticObjectCells* m_staticObjsCells = nullptr;

public:
	UniqueGridScene2D(Engine* engine, size_t width, size_t height, float cellWidth, float cellHeight);
	~UniqueGridScene2D();

	// Inherited via Scene2D
	virtual void AddStaticObject(GameObject2D* obj) override;

	virtual void RemoveStaticObject(GameObject2D* obj) override;

	virtual void AddDynamicObject(GameObject2D* obj) override;

	virtual void RemoveDynamicObject(GameObject2D* obj) override;

	virtual void ReConstruct() override;

	virtual Scene2DQuerySession* NewQuerySession() override;

	virtual void DeleteQuerySession(Scene2DQuerySession*) override;

	virtual void AABBStaticQueryAARect(const AARect& aaRect, Scene2DQuerySession* session) override;

	virtual void AABBDynamicQueryAARect(const AARect& aaRect, Scene2DQuerySession* session) override;

};

NAMESPACE_END