#pragma once

#include "Rendering2D.h"

#include "Math/Math.h"

#include "Objects2D/GameObject2D.h"

#include "Graphics2D/Graphics2D.h"

#include "SubSystems2D/Rendering/RenderingSystem2D.h"


NAMESPACE_BEGIN

class Renderer2D : public Rendering2D
{
protected:
	// get scale that scale sprite to fit the expectSize
	inline Vec2 GetScaleFitTo(sf::Sprite& sprite, const Vec2& expectSize)
	{
		auto& textureRect = sprite.getTextureRect();
		return {
			expectSize.x / (float)textureRect.width,
			expectSize.y / (float)textureRect.height
		};
	}

	inline void SetSpriteTextureRect(sf::Sprite& sprite, const AARect& rect)
	{
		if (rect.IsValid())
		{
			sprite.setTextureRect({
				(int)rect.m_topLeft.x,
				(int)rect.m_topLeft.y,
				(int)rect.m_dimensions.x,
				(int)rect.m_dimensions.y
			});
		}
	}

	inline void RenderSpriteWithObject(RenderingSystem2D* rdr, sf::Sprite& sprite, const Vec2& originScale = { 1,1 })
	{
		sprite.setPosition(reinterpret_cast<const sf::Vector2f&>(m_object->GlobalTransform().GetTranslation()));
		sprite.setRotation(ToDegrees(m_object->GlobalTransform().GetRotation()));
		sprite.setScale(reinterpret_cast<sf::Vector2f&>(m_object->GlobalTransform().GetScale() * originScale));
		rdr->DrawSprite(sprite);
	};

	inline void RenderSprite(RenderingSystem2D* rdr, sf::Sprite& sprite)
	{
		rdr->DrawSprite(sprite);
	};

	inline void RenderSprite(RenderingSystem2D* rdr, sf::Sprite& sprite, 
		const Vec2& scale, float rotation, const Vec2& position)
	{
		sprite.setPosition(reinterpret_cast<const sf::Vector2f&>(position));
		sprite.setRotation(ToDegrees(rotation));
		sprite.setScale(reinterpret_cast<const sf::Vector2f&>(scale));
		rdr->DrawSprite(sprite);
	};

	inline void RenderSprite(RenderingSystem2D* rdr, sf::Sprite& sprite,
		float rotation, const Vec2& position)
	{
		sprite.setPosition(reinterpret_cast<const sf::Vector2f&>(position));
		sprite.setRotation(ToDegrees(rotation));
		rdr->DrawSprite(sprite);
	};

};

NAMESPACE_END