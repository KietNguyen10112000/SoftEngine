#pragma once

#include "Renderer2D.h"

#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class SpritesRenderer : public Renderer2D
{
protected:
	friend class RenderingSystem2D;

	std::Vector<Sprite> m_sprites;
	size_t m_currentId = INVALID_ID;

public:
	inline SpritesRenderer(size_t capacity = 16)
	{
		m_sprites.reserve(capacity);
	};

public:
	inline ID Load(String path, const AARect& rect = {}, const Vec2& rectSize = {})
	{
		m_sprites.emplace_back();
		auto& s = m_sprites.back();
		s.Initialize(path, rect, {});

		Transform2D originTransform = {};
		originTransform.Scale() = GetScaleFitTo(s.SFSprite(), rectSize);
		s.Transform() = originTransform;

		return m_sprites.size() - 1;
	}

	inline ID Load(String path, const AARect& rect = {}, const Transform2D& transform = {})
	{
		m_sprites.emplace_back();
		auto& s = m_sprites.back();
		s.Initialize(path, rect, transform);
		return m_sprites.size() - 1;
	}

	inline void SetSprite(ID id)
	{
		m_currentId = id;
	}

	inline auto& Sprite(ID id)
	{
		return m_sprites[id];
	}

	virtual void Render(RenderingSystem2D* rdr) override
	{
		if (m_currentId == INVALID_ID)
		{
			return;
		}

		auto& s = m_sprites[m_currentId];
		RenderSpriteWithObject(rdr, s);
	};

	virtual AARect GetLocalAABB() override
	{
		if (m_currentId == INVALID_ID)
		{
			return {};
		}

		return m_sprites[m_currentId].GetLocalAABB();
	}

};

NAMESPACE_END