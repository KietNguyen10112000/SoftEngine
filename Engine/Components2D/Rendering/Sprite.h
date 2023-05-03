#pragma once

#include "Rendering2D.h"

#include "Math/Math.h"

#include "Objects2D/GameObject2D.h"
//#include "Objects2D/Scene2D/Scene2D.h"

#include "Graphics2D/Graphics2D.h"

#include "FileSystem/FileUtils.h"

NAMESPACE_BEGIN

class Sprite : public Rendering2D
{
protected:
	friend class RenderingSystem2D;

	sf::Sprite m_sprite;
	sf::Texture m_texture;

public:
	inline Sprite(const char* path, const AARect& rect = {})
	{
		m_texture.loadFromFile(path);
		m_sprite.setTexture(m_texture);

		if (rect.IsValid())
		{
			m_sprite.setTextureRect({ 
				(int)rect.m_topLeft.x, 
				(int)rect.m_topLeft.y,
				(int)rect.m_dimensions.x,
				(int)rect.m_dimensions.y
			});
		}
		
	};

public:
	virtual void OnComponentAddedToScene() final override {}

	virtual void OnComponentAdded() override {}

	virtual void OnComponentRemoved() override {}

	virtual void OnComponentRemovedFromScene() override {}

	virtual void OnObjectRefresh() override {};

	virtual void SetAsMain() override {}

	virtual void SetAsExtra() override {}

	virtual void ResolveBranch() override {}

	virtual bool IsNewBranch() override { return false; }

	virtual void Render(RenderingSystem2D* rdr) override 
	{
		auto& sf = Graphics2D::Get()->GetSFWindow();
		m_sprite.setPosition(reinterpret_cast<sf::Vector2f&>(m_object->Position()));
		m_sprite.setRotation(m_object->Rotation());
		m_sprite.setScale(reinterpret_cast<sf::Vector2f&>(m_object->Scale()));
		sf.draw(m_sprite);
	};

	virtual AARect GetLocalAABB() override
	{
		auto& rect = m_sprite.getTextureRect();
		return AARect(
			{ 0,0 },
			{ rect.width, rect.height }
		);
	}

};

NAMESPACE_END