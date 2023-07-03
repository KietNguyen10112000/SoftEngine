#pragma once

#include "SubSystems2D/SubSystem2D.h"

#include "Objects2D/Scene2D/Scene2D.h"
#include "Objects2D/Rendering/Sprite.h"

#include "SFML/Graphics.hpp"


NAMESPACE_BEGIN

class Scene2D;
class Camera2D;
class Rendering2D;

class RenderingSystem2D : public SubSystem2D
{
private:
	std::Vector<GameObject2D*> m_cameraObjects;

	std::Vector<GameObject2D*> m_ghostObjects;
	bool m_refreshGhostObject = false;

	Scene2DQuerySession* m_querySession;

	sf::RenderWindow* m_bindedWindow = nullptr;
	Camera2D* m_bindedCamera = nullptr;

	std::Vector<Rendering2D*> m_renderList;

public:
	RenderingSystem2D(Scene2D* scene);
	~RenderingSystem2D();

public:
	virtual void PrevIteration(float dt) override;
	virtual void Iteration(float dt) override;
	virtual void PostIteration(float dt) override;
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) override;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) override;

public:
	void AddCamera(Camera2D* cam);

	inline auto& GetSFWindow()
	{
		return *m_bindedWindow;
	}

	inline auto GetCurrentCamera()
	{
		return m_bindedCamera;
	}

	inline void DrawSprite(sf::Sprite& sprite)
	{
		m_bindedWindow->draw(sprite);
	}

	inline void DrawSF(sf::Drawable& drawable)
	{
		m_bindedWindow->draw(drawable);
	}

	inline void DrawSprite(Sprite& sprite)
	{
		auto& originTransform = sprite.Transform();

		auto& scale = originTransform.GetScale();
		auto& rotation = originTransform.GetRotation();
		auto& translation = originTransform.GetTranslation();

		auto& sfSprite = sprite.SFSprite();
		sfSprite.setPosition(reinterpret_cast<const sf::Vector2f&>(translation));
		sfSprite.setRotation(ToDegrees(rotation));
		sfSprite.setScale(reinterpret_cast<const sf::Vector2f&>(scale));
		DrawSprite(sfSprite);
	};

};

NAMESPACE_END