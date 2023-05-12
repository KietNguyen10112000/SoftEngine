#pragma once

#include "Renderer2D.h"

#include "Resources/Texture2D.h"

#include "Objects2D/Rendering/Sprite.h"

NAMESPACE_BEGIN

class SpriteRenderer : public Renderer2D
{
protected:
	friend class RenderingSystem2D;

	Sprite m_sprite;

public:
	inline SpriteRenderer(String path, const AARect& rect = {}, const Transform2D& transform = {})
		: m_sprite(path, rect, transform)
	{

	};

	// =)))
	inline void ClearAABB()
	{
		m_sprite.SetAABBSize({0,0});
	}

	inline auto& Sprite()
	{
		return m_sprite;
	}

public:
	virtual void Render(RenderingSystem2D* rdr) override 
	{
		RenderSpriteWithObject(rdr, m_sprite);
	};

	virtual AARect GetLocalAABB() override
	{
		return m_sprite.GetLocalAABB();
	}

};

NAMESPACE_END