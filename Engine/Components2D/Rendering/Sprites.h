#pragma once

#include "Renderer2D.h"

#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class Sprites : public Renderer2D
{
protected:
	friend class RenderingSystem2D;

	struct LoadedSprite
	{
		Resource<Texture2D> texture;
		sf::Sprite sprite;
		Vec2 originScale = { 1,1 };
		Vec2 size = {};
	};

	std::Vector<LoadedSprite> m_sprites;
	size_t m_currentId = -INVALID_ID;

public:
	inline Sprites(size_t capacity = 16)
	{
		m_sprites.reserve(capacity);
	};

public:
	inline ID Load(String path, const AARect& rect = {}, const Vec2& size = { 0,0 })
	{
		m_sprites.emplace_back();
		auto& s = m_sprites.back();

		s.texture = resource::Load<Texture2D>(path);

		s.sprite.setTexture(s.texture->GetSFTexture());

		SetSpriteTextureRect(s.sprite, rect);

		auto& textureRect = s.sprite.getTextureRect();
		s.size.x = textureRect.width;
		s.size.y = textureRect.height;

		if (size.x != 0 && size.y != 0)
		{
			s.originScale.x = size.x / (float)textureRect.width;
			s.originScale.y = size.y / (float)textureRect.height;

			s.size = size;
		}

		return m_sprites.size() - 1;
	}

	inline void SetSprite(ID id)
	{
		m_currentId = id;
	}

	virtual void Render(RenderingSystem2D* rdr) override
	{
		if (m_currentId == INVALID_ID)
		{
			return;
		}

		auto& s = m_sprites[m_currentId];
		RenderSpriteWithObject(rdr, s.sprite, s.originScale);
	};

	virtual AARect GetLocalAABB() override
	{
		if (m_currentId == INVALID_ID)
		{
			return {};
		}

		return AARect(
			{ 0,0 },
			m_sprites[m_currentId].size
		);
	}

};

NAMESPACE_END