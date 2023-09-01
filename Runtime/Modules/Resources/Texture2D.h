#pragma once
#include "Resource.h"

#include "SFML/Graphics.hpp"

NAMESPACE_BEGIN

class Texture2D : public ResourceBase
{
protected:
	sf::Texture m_texture;

public:
	Texture2D(String path) : ResourceBase(path)
	{
		resource::ReadFile(path,
			[&](byte* buffer, size_t size)
			{
				m_texture.loadFromMemory(buffer, size);
			}
		);
	}

public:
	inline auto& GetSFTexture() const
	{
		return m_texture;
	}

};

NAMESPACE_END