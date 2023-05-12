#pragma once

#include "Resources/Texture2D.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class Sprite
{
protected:
	Resource<Texture2D> m_texture;
	sf::Sprite m_sprite;
	Transform2D m_transform;
	Vec2 m_anchorPoint;
	Vec2 m_size;

public:
	inline Sprite() {};

	inline Sprite(String path, const AARect& rect = {}, const Transform2D& transform = {})
	{
		Initialize(path, rect, transform);
	};

	inline void Initialize(String path, const AARect& rect, const Transform2D& transform)
	{
		m_texture = resource::Load<Texture2D>(path);

		m_sprite.setTexture(m_texture->GetSFTexture());
		SetSize({});

		if (rect.IsValid())
		{
			m_sprite.setTextureRect({
				(int)rect.m_topLeft.x,
				(int)rect.m_topLeft.y,
				(int)rect.m_dimensions.x,
				(int)rect.m_dimensions.y
			});
		}

		m_transform = transform;
		m_anchorPoint = { 0,0 };
	}

	// [0, 255]
	inline void SetOpacity(byte opacity)
	{
		m_sprite.setColor(sf::Color(255, 255, 255, opacity));
	}

	// [0, 255]
	inline void SetColor(byte r, byte g, byte b, byte a = 255)
	{
		m_sprite.setColor(sf::Color(r, g, b, a));
	}

	// anchor point in percents, default is (0,0)
	inline void SetAnchorPoint(const Vec2& anchorPoint)
	{
		m_anchorPoint = anchorPoint;
		auto& rect = m_sprite.getTextureRect();
		m_sprite.setOrigin(reinterpret_cast<const sf::Vector2f&>(anchorPoint * Vec2(rect.width, rect.height)));
	}

	inline auto& Transform()
	{
		return m_transform;
	}

	inline void SetSize(const Vec2& size)
	{
		auto& textureRect = m_sprite.getTextureRect();
		m_size.x = textureRect.width;
		m_size.y = textureRect.height;

		if (size.x != 0 && size.y != 0)
		{
			m_transform.Scale().x = size.x / (float)textureRect.width;
			m_transform.Scale().y = size.y / (float)textureRect.height;

			m_size = size;
		}
	}

	inline void SetAABBSize(const Vec2& size)
	{
		m_size = size;
	}

	inline void FitTextureSize(const Vec2& expectSize)
	{
		/*auto& textureRect = m_sprite.getTextureRect();
		m_size.x = expectSize.x / (float)textureRect.width;
		m_size.y = expectSize.y / (float)textureRect.height;*/
		SetSize(expectSize);
	}

public:
	inline auto& SFSprite()
	{
		return m_sprite;
	}

	AARect GetLocalAABB()
	{
		AARect rect;
		rect.m_topLeft = -m_anchorPoint * m_size;
		rect.m_dimensions = m_size;
		rect.Transform(m_transform.ToTransformMatrix());
		return rect;
	}

};

NAMESPACE_END