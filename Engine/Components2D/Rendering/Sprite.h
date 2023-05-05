#pragma once

#include "Renderer2D.h"

#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class Sprite : public Renderer2D
{
protected:
	friend class RenderingSystem2D;

	Resource<Texture2D> m_texture;
	sf::Sprite m_sprite;
	Vec2 m_originScale = { 1,1 };
	Vec2 m_size = {};

public:
	inline Sprite(String path, const AARect& rect = {}, const Vec2& size = { 0,0 })
	{
		m_texture = resource::Load<Texture2D>(path);

		m_sprite.setTexture(m_texture->GetSFTexture());

		SetSpriteTextureRect(m_sprite, rect);

		auto& textureRect = m_sprite.getTextureRect();
		m_size.x = textureRect.width;
		m_size.y = textureRect.height;
		
		if (size.x != 0 && size.y != 0)
		{
			m_originScale.x = size.x / (float)textureRect.width;
			m_originScale.y = size.y / (float)textureRect.height;

			m_size = size;
		}
	};

public:
	virtual void Render(RenderingSystem2D* rdr) override 
	{
		/*auto& sf = Graphics2D::Get()->GetSFWindow();
		m_sprite.setPosition(reinterpret_cast<sf::Vector2f&>(m_object->Position()));
		m_sprite.setRotation(m_object->Rotation());
		m_sprite.setScale(reinterpret_cast<sf::Vector2f&>(m_object->Scale() * m_originScale));
		sf.draw(m_sprite);*/

		RenderSpriteWithObject(rdr, m_sprite, m_originScale);
	};

	virtual AARect GetLocalAABB() override
	{
		return AARect(
			{ 0,0 },
			m_size
		);
	}

};

NAMESPACE_END