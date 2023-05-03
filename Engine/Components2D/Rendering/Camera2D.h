#pragma once

#include "Rendering2D.h"

#include "Math/Math.h"

#include "Objects2D/GameObject2D.h"
#include "Objects2D/Scene2D/Scene2D.h"

#include "SubSystems2D/Rendering/RenderingSystem2D.h"

NAMESPACE_BEGIN

class Camera2D : public Rendering2D
{
protected:
	friend class RenderingSystem2D;

	size_t m_id;
	AARect m_rect;
	AARect m_viewPort;
	size_t m_numRenderObjects = 0;

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

};

NAMESPACE_END