#pragma once

#include "Renderer2D.h"

NAMESPACE_BEGIN

class LineRenderer : public Renderer2D
{
protected:
	friend class RenderingSystem2D;

	sf::Vector2f m_vertices[2];

public:
	inline LineRenderer()
	{
		
	};

public:
	inline void Set(const Vec2& start, const Vec2& end)
	{
		m_vertices[0] = reinterpret_cast<const sf::Vector2f&>(start);
		m_vertices[1] = reinterpret_cast<const sf::Vector2f&>(end);
	}

	virtual void Render(RenderingSystem2D* rdr) override
	{
		rdr->GetSFWindow().draw(m_vertices, 2, sf::Lines);
	};

	virtual AARect GetLocalAABB() override
	{
		return {};
	}

};

NAMESPACE_END