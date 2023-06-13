#include "UniqueGridScene2D.h"

#include "Objects2D/GameObject2D.h"

NAMESPACE_BEGIN

class UniqueGridScene2DQuerySession : public Scene2DQuerySession
{
public:
	std::Vector<GameObject2D*> m_result;

	UniqueGridScene2DQuerySession()
	{
		m_result.reserve(8 * KB);
	}

	// Inherited via Scene2DQuerySession
	virtual void Clear() override
	{
		m_result.clear();
		m_begin = 0;
		m_end = 0;
	}

};

UniqueGridScene2D::UniqueGridScene2D(Engine* engine, size_t width, size_t height, float cellWidth, float cellHeight)
	: Scene2D(engine), m_width(width), m_height(height)
{
	m_cellDimensions = { cellWidth, cellHeight };
	m_cells.resize(width * height);
}

UniqueGridScene2D::~UniqueGridScene2D()
{
	Dtor();

	auto it = m_staticObjsCells;
	while (it)
	{
		auto next = it->next;
		rheap::Delete((size_t*)it);
		it = next;
	}
}

void UniqueGridScene2D::AddStaticObject(GameObject2D* obj)
{
	Vec2 temp[4];
	auto& aabb = obj->GetAABB();
	aabb.GetPoints(temp);

	auto beginX		= (intmax_t)std::floor(temp[0].x / m_cellDimensions.x);
	auto endX		= (intmax_t)std::ceil(temp[1].x / m_cellDimensions.x);

	auto beginY		= (intmax_t)std::floor(temp[0].y / m_cellDimensions.y);
	auto endY		= (intmax_t)std::ceil(temp[2].y / m_cellDimensions.y);

	if (beginX < 0 || endX > m_width || beginY < 0 || endY > m_height)
	{
		obj->m_aabbQueryId = STATIC_OUTSIDE;
		obj->m_sceneDynamicId = m_outsideStaticObjects.size();
		m_outsideStaticObjects.push_back(obj);
		return;
	}

	auto list = rheap::NewArray<size_t>(
			sizeof(StaticObjectCells) / sizeof(size_t) + (endX - beginX) * (endY - beginY) * 2 + 2
		);

	obj->m_aabbQueryId = STATIC_INSIDE;
	obj->m_sceneDynamicId = (ID)list;

	size_t i = 0;

	auto sCells = (StaticObjectCells*)list;
	sCells->prev = nullptr;
	sCells->next = m_staticObjsCells;
	m_staticObjsCells = sCells;
	list += 2;
	for (size_t y = beginY; y < endY; y++)
	{
		for (size_t x = beginX; x < endX; x++)
		{
			auto idx = x + y * m_width;
			auto& objs = m_cells[idx].staticObjects;

			list[i++] = idx;
			list[i++] = objs.size();

			objs.push_back(obj);
		}
	}

	list[i++] = -1;
	list[i++] = -1;
}

#define ROLL_TO_FILL_BLANK(v, objName, idName)				\
if (v.size() == 1)											\
{															\
	v.clear();												\
	return;													\
}															\
auto& blank = v[objName->idName];							\
auto& back = v.back();										\
back->idName = objName->idName;								\
blank = back;												\
v.pop_back();

void UniqueGridScene2D::RemoveStaticObject(GameObject2D* obj)
{
	if (obj->m_aabbQueryId == STATIC_INSIDE)
	{
		auto list = (size_t*)obj->m_sceneDynamicId;

		auto sCells = (StaticObjectCells*)list;
		auto prev = sCells->prev;
		auto next = sCells->next;

		if (prev)
		{
			prev->next = next;
		}
		else
		{
			m_staticObjsCells = sCells;
		}

		if (next)
		{
			next->prev = prev;
		}

		list += 2;
		size_t i = 0;
		while (list[i] != -1)
		{
			auto cellIdx = list[i++];
			auto idx = list[i++];
			auto& cell = m_cells[cellIdx];

			if (cell.staticObjects.size() == 0)
			{
				cell.staticObjects.clear();
				continue;
			}

			auto& blank = cell.staticObjects[idx];
			auto back = cell.staticObjects.back();

			size_t backI = 0;
			auto backList = (size_t*)back->m_sceneDynamicId;
			backList += 2;
			while (backList[backI] != -1)
			{
				auto& backCellIdx = backList[backI++];
				auto& backIdx = backList[backI++];
				if (backCellIdx == cellIdx)
				{
					backIdx = idx;
					break;
				}
			}
			assert(backList[backI] != -1);
			
			blank = back;
			cell.staticObjects.pop_back();
		}

		rheap::Delete(list);
	}

	if (obj->m_aabbQueryId == STATIC_OUTSIDE)
	{
		ROLL_TO_FILL_BLANK(m_outsideStaticObjects, obj, m_sceneDynamicId);
	}
}

void UniqueGridScene2D::AddDynamicObject(GameObject2D* obj)
{
	obj->m_aabbQueryId = DYNAMIC_INSIDE;
	obj->m_sceneDynamicId = m_dynamicObjects.size();
	m_dynamicObjects.push_back(obj);
}

void UniqueGridScene2D::RemoveDynamicObject(GameObject2D* obj)
{
	ROLL_TO_FILL_BLANK(m_dynamicObjects, obj, m_sceneDynamicId);
}

void UniqueGridScene2D::ReConstruct()
{
	m_outsideDynamicObjects.clear();

	Vec2 temp[4];
	for (auto& obj : m_dynamicObjects)
	{
		if (obj->IsMovedRecursive())
		{
			obj->RecalculateAABB();
		}

		auto& aabb = obj->GetAABB();
		aabb.GetPoints(temp);

		auto beginX			= (intmax_t)std::floor(temp[0].x / m_cellDimensions.x);
		auto endX			= (intmax_t)std::ceil(temp[1].x / m_cellDimensions.x);

		auto beginY			= (intmax_t)std::floor(temp[0].y / m_cellDimensions.y);
		auto endY			= (intmax_t)std::ceil(temp[2].y / m_cellDimensions.y);

		if (beginX < 0 || endX > m_width || beginY < 0 || endY > m_height)
		{
			m_outsideDynamicObjects.push_back(obj);
			continue;
		}

		for (size_t y = beginY; y < endY; y++)
		{
			for (size_t x = beginX; x < endX; x++)
			{
				auto idx = x + y * m_width;
				auto& cell = m_cells[idx];

				if (cell.lastClearIteration != m_iterationCount)
				{
					cell.dynamicObjects.clear();
					cell.lastClearIteration = m_iterationCount;
				}

				cell.dynamicObjects.push_back(obj);
			}
		}
	}
}

Scene2DQuerySession* UniqueGridScene2D::NewQuerySession()
{
	return rheap::New<UniqueGridScene2DQuerySession>();
}

void UniqueGridScene2D::DeleteQuerySession(Scene2DQuerySession* ss)
{
	rheap::Delete((UniqueGridScene2DQuerySession*)ss);
}

void UniqueGridScene2D::AABBStaticQueryAARect(const AARect& aaRect, Scene2DQuerySession* session)
{
	auto ss = (UniqueGridScene2DQuerySession*)session;
	auto& ret = ss->m_result;

	Vec2 temp[4];
	auto& aabb = aaRect;
	aabb.GetPoints(temp);

	auto beginX		= (intmax_t)std::floor(temp[0].x / m_cellDimensions.x);
	auto endX		= (intmax_t)std::ceil(temp[1].x / m_cellDimensions.x);

	auto beginY		= (intmax_t)std::floor(temp[0].y / m_cellDimensions.y);
	auto endY		= (intmax_t)std::ceil(temp[2].y / m_cellDimensions.y);

	beginX			= clamp(beginX, (intmax_t)0, (intmax_t)m_width);
	endX			= clamp(endX, (intmax_t)0, (intmax_t)m_width);

	beginY			= clamp(beginY, (intmax_t)0, (intmax_t)m_height);
	endY			= clamp(endY, (intmax_t)0, (intmax_t)m_height);

	for (size_t y = beginY; y < endY; y++)
	{
		for (size_t x = beginX; x < endX; x++)
		{
			auto idx = x + y * m_width;
			auto& cell = m_cells[idx];
			
			//ret.insert(ret.end(), cell.staticObjects.begin(), cell.staticObjects.end());
			for (auto& obj : cell.staticObjects)
			{
				if (obj->m_queried == 0)
				{
					ret.push_back(obj);
					obj->m_queried = 1;
				}
			}
		}
	}

	for (auto& obj : ret)
	{
		obj->m_queried = 0;
	}

	ret.insert(ret.end(), m_outsideStaticObjects.begin(), m_outsideStaticObjects.end());
	ss->m_begin = ret.data();
	ss->m_end = ret.data() + ret.size();

#ifdef _DEBUG
	for (auto& e : ret)
	{
		assert(e->m_queried == 0);
		e->m_queried = 1;
	}
	for (auto& e : ret)
	{
		e->m_queried = 0;
	}
#endif // _DEBUG
}

void UniqueGridScene2D::AABBDynamicQueryAARect(const AARect& aaRect, Scene2DQuerySession* session)
{
	auto ss = (UniqueGridScene2DQuerySession*)session;
	auto& ret = ss->m_result;

	Vec2 temp[4];
	auto& aabb = aaRect;
	aabb.GetPoints(temp);

	auto beginX			= (intmax_t)std::floor(temp[0].x / m_cellDimensions.x);
	auto endX			= (intmax_t)std::ceil(temp[1].x / m_cellDimensions.x);

	auto beginY			= (intmax_t)std::floor(temp[0].y / m_cellDimensions.y);
	auto endY			= (intmax_t)std::ceil(temp[2].y / m_cellDimensions.y);

	beginX				= clamp(beginX, (intmax_t)0, (intmax_t)m_width);
	endX				= clamp(endX, (intmax_t)0, (intmax_t)m_width);

	beginY				= clamp(beginY, (intmax_t)0, (intmax_t)m_height);
	endY				= clamp(endY, (intmax_t)0, (intmax_t)m_height);

	for (size_t y = beginY; y < endY; y++)
	{
		for (size_t x = beginX; x < endX; x++)
		{
			auto idx = x + y * m_width;
			auto& cell = m_cells[idx];

			if (cell.lastClearIteration != m_iterationCount)
			{
				cell.dynamicObjects.clear();
				cell.lastClearIteration = m_iterationCount;
				continue;
			}

			//ret.insert(ret.end(), cell.dynamicObjects.begin(), cell.dynamicObjects.end());

			for (auto& obj : cell.dynamicObjects)
			{
				if (obj->m_queried == 0)
				{
					ret.push_back(obj);
					obj->m_queried = 1;
				}
			}
		}
	}

	for (auto& obj : ret)
	{
		obj->m_queried = 0;
	}

	ret.insert(ret.end(), m_outsideDynamicObjects.begin(), m_outsideDynamicObjects.end());
	ss->m_begin = ret.data();
	ss->m_end = ret.data() + ret.size();

#ifdef _DEBUG
	for (auto& e : ret)
	{
		assert(e->m_queried == 0);
		e->m_queried = 1;
	}
	for (auto& e : ret)
	{
		e->m_queried = 0;
	}
#endif // _DEBUG

}

NAMESPACE_END