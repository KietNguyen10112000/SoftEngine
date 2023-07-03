#pragma once

#include "Rendering2D.h"

#include "Math/Math.h"

#include "Objects2D/GameObject2D.h"
#include "Objects2D/Rendering/Sprite.h"

#include "Graphics2D/Graphics2D.h"

#include "SubSystems2D/Rendering/RenderingSystem2D.h"


NAMESPACE_BEGIN

class API Renderer2D : public Rendering2D
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

	inline void RenderSpriteWithObject(RenderingSystem2D* rdr, Sprite& sprite)
	{
		auto& originTransform = sprite.Transform();

		auto scale			= originTransform.GetScale() * m_object->GlobalTransform().GetScale();
		auto rotation		= originTransform.GetRotation() + m_object->GlobalTransform().GetRotation();
		auto translation	= originTransform.GetTranslation() + m_object->GlobalTransform().GetTranslation();

		auto& sfSprite = sprite.SFSprite();
		sfSprite.setPosition(reinterpret_cast<const sf::Vector2f&>(translation));
		sfSprite.setRotation(ToDegrees(rotation));
		sfSprite.setScale(reinterpret_cast<sf::Vector2f&>(scale));
		rdr->DrawSprite(sfSprite);
	};

public:
	inline static void RenderSprite(RenderingSystem2D* rdr, Sprite& sprite, 
		const Vec2& scale, float rotation, const Vec2& position)
	{
		auto& originTransform = sprite.Transform();

		auto _scale = originTransform.GetScale() * scale;
		auto _rotation = originTransform.GetRotation() + rotation;
		auto _translation = originTransform.GetTranslation() + position;

		auto& sfSprite = sprite.SFSprite();
		sfSprite.setPosition(reinterpret_cast<const sf::Vector2f&>(_translation));
		sfSprite.setRotation(ToDegrees(_rotation));
		sfSprite.setScale(reinterpret_cast<const sf::Vector2f&>(_scale));
		rdr->DrawSprite(sfSprite);
	};

	inline static void RenderSprite(RenderingSystem2D* rdr, Sprite& sprite)
	{
		auto& originTransform = sprite.Transform();

		auto& _scale = originTransform.GetScale();
		auto& _rotation = originTransform.GetRotation();
		auto& _translation = originTransform.GetTranslation();

		auto& sfSprite = sprite.SFSprite();
		sfSprite.setPosition(reinterpret_cast<const sf::Vector2f&>(_translation));
		sfSprite.setRotation(ToDegrees(_rotation));
		sfSprite.setScale(reinterpret_cast<const sf::Vector2f&>(_scale));
		rdr->DrawSprite(sfSprite);
	};

	inline void FlushTransform(sf::Transformable& sfObj)
	{
		auto& scale = m_object->GlobalTransform().GetScale();
		auto& rotation = m_object->GlobalTransform().GetRotation();
		auto& translation = m_object->GlobalTransform().GetTranslation();
		sfObj.setPosition(reinterpret_cast<const sf::Vector2f&>(translation));
		sfObj.setRotation(ToDegrees(rotation));
		sfObj.setScale(reinterpret_cast<const sf::Vector2f&>(scale));
	}

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


	inline static void RenderSprite(sf::RenderTexture& texture, sf::Sprite& sprite,
		const Vec2& scale, float rotation, const Vec2& position)
	{
		sprite.setPosition(reinterpret_cast<const sf::Vector2f&>(position));
		sprite.setRotation(ToDegrees(rotation));
		sprite.setScale(reinterpret_cast<const sf::Vector2f&>(scale));
		texture.draw(sprite);
	};

	inline static void RenderSprite(sf::RenderTexture& texture, sf::Sprite& sprite,
		float rotation, const Vec2& position)
	{
		sprite.setPosition(reinterpret_cast<const sf::Vector2f&>(position));
		sprite.setRotation(ToDegrees(rotation));
		texture.draw(sprite);
	};

};

NAMESPACE_END