#pragma once

#include "../Rendering.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class RenderingSystem;

class Renderer : public Rendering
{
public:
	inline virtual ~Renderer() {};

public:
	virtual void SetAsMain() final override {}

	virtual void SetAsExtra() final override {}

	virtual void ResolveBranch() final override {}

	virtual bool IsNewBranch() final override { return false; }

	virtual void OnObjectRefresh() override {};

};

NAMESPACE_END