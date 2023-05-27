#pragma once

#include "Renderer2D.h"

#include "Resources/Texture2D.h"

#include "Objects2D/Rendering/AnimatedSprites.h"

NAMESPACE_BEGIN

class AnimatedSpritesRenderer : public Renderer2D, public AnimatedSprites
{
protected:
	friend class RenderingSystem2D;

	bool m_clearedAABB = false;

public:
	// =)))
	inline void ClearAABB()
	{
		m_clearedAABB = true;
	}

public:
	virtual void Render(RenderingSystem2D* rdr) override
	{
		auto dt = GetObject()->GetScene()->Dt();
		Play(dt);
		RenderSpriteWithObject(rdr, GetCurrentSpriteFrame());
	};

	virtual AARect GetLocalAABB() override
	{
		if (m_clearedAABB) return AARect({ 0,0 }, { 0,0 });
		return GetCurrentSpriteFrame().GetLocalAABB();
	}

};

NAMESPACE_END