#pragma once

#include "Rendering2D.h"

#include "Math/Math.h"

#include "Objects2D/GameObject2D.h"
#include "Objects2D/Scene2D/Scene2D.h"

#include "SubSystems2D/Rendering/RenderingSystem2D.h"

NAMESPACE_BEGIN

class API Camera2D : public Rendering2D
{
protected:
	friend class RenderingSystem2D;

	size_t m_id;
	AARect m_rect;
	AARect m_viewPort;
	size_t m_numRenderObjects = 0;

	Vec2 m_posMin = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
	Vec2 m_posMax = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };

public:
	inline Camera2D(const AARect& rect) : m_rect(rect) {};

public:
	virtual void OnComponentAddedToScene() final override 
	{
		GetObject()->GetScene()->GetRenderingSystem()->AddCamera(this);
	}

	virtual void OnComponentAdded() override {}

	virtual void OnComponentRemoved() override {}

	virtual void OnComponentRemovedFromScene() override {}

	virtual void OnObjectRefresh() override {};

	virtual void SetAsMain() override {}

	virtual void SetAsExtra() override {}

	virtual void ResolveBranch() override {}

	virtual bool IsNewBranch() override { return false; }

	virtual void Render(RenderingSystem2D* rdr) override {};

public:
	inline size_t& NumRenderObjects()
	{
		return m_numRenderObjects;
	}

	inline auto& Rect()
	{
		return m_rect;
	}

	inline auto& ViewPort()
	{
		return m_viewPort;
	}

	inline auto GetView()
	{
		auto& pos = GetObject()->GlobalTransform().GetTranslation();
		Vec2 lastPos = { clamp(pos.x, m_posMin.x, m_posMax.x), clamp(pos.y, m_posMin.y, m_posMax.y) };
		lastPos.Round();
		AARect view = AARect(lastPos - Rect().GetDimensions() / 2.0f, Rect().GetDimensions());
		return view;
	}

	inline auto GetCenter()
	{
		auto& pos = GetObject()->GlobalTransform().GetTranslation();
		Vec2 lastPos = { clamp(pos.x, m_posMin.x, m_posMax.x), clamp(pos.y, m_posMin.y, m_posMax.y) };
		lastPos.Round();
		return lastPos;
	}

	inline auto GetWorldPosition(const Vec2& screenPosition, int screenWidth, int screenHeight)
	{
		auto view = GetView();
		auto px = screenPosition.x / (float)screenWidth;
		auto py = screenPosition.y / (float)screenHeight;
		return Vec2(px * view.GetDimensions().x, py * view.GetDimensions().y) + view.GetTopLeft();
	}

	inline auto GetScreenPosition(const Vec2& worldPosition, int screenWidth, int screenHeight)
	{
		auto view = GetView();
		return worldPosition - view.GetTopLeft();
	}

	inline auto SetClamp(const Vec2& posMin, const Vec2& posMax)
	{
		m_posMin = posMin;
		m_posMax = posMax;
	}

};

NAMESPACE_END