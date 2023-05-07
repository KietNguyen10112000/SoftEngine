#pragma once

#include "Core/TypeDef.h"
#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

NAMESPACE_BEGIN

class RenderingSystem2D;

class Rendering2D : public SubSystemComponent2D
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId2D::RENDERING_SUBSYSTEM_COMPONENT_ID;

	friend class RenderingSystem2D;
	friend class GameObject2D;
	friend class Scene2D;

private:
	ID m_id = INVALID_ID;

protected:
	intmax_t m_zOrder = 0;

public:
	inline Rendering2D() {};
	inline virtual ~Rendering2D() {};

protected:
	virtual void OnComponentAddedToScene() override {}

	virtual void OnComponentAdded() override {}

	virtual void OnComponentRemoved() override {}

	virtual void OnComponentRemovedFromScene() override {}

	virtual void OnObjectRefresh() override {};

	virtual void SetAsMain() override {}

	virtual void SetAsExtra() override {}

	virtual void ResolveBranch() override {}

	virtual bool IsNewBranch() override { return false; }

public:
	// write data for render
	// and call render call
	virtual void Render(RenderingSystem2D* rdr) = 0;

	inline auto& ZOrder()
	{
		return m_zOrder;
	}

};

NAMESPACE_END