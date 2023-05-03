#include "RenderingSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"

#include "Components2D/Rendering/Camera2D.h"

#include "Graphics2D/Graphics2D.h"

#include <iostream>


NAMESPACE_BEGIN

RenderingSystem2D::RenderingSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	m_querySession = scene->NewQuerySession();
}

RenderingSystem2D::~RenderingSystem2D()
{
	m_scene->DeleteQuerySession(m_querySession);
}

void RenderingSystem2D::PrevIteration(float dt)
{
}

void RenderingSystem2D::Iteration(float dt)
{
	auto& window = Graphics2D::Get()->m_window;
	window.clear(sf::Color::Black);

	if (m_cameraObjects.size() != 0)
	{
		auto camObj = m_cameraObjects[0];
		auto cam = m_cameraObjects[0]->GetComponentRaw<Camera2D>();
		auto& rect = cam->Rect();

		sf::View view1;
		view1.setCenter(reinterpret_cast<sf::Vector2f&>(camObj->Position()));
		view1.setSize(reinterpret_cast<sf::Vector2f&>(rect.GetDimensions()));
		window.setView(view1);

		m_querySession->Clear();
		m_scene->AABBStaticQueryAARect(rect, m_querySession);
		m_scene->AABBDynamicQueryAARect(rect, m_querySession);
		for (auto obj : *m_querySession)
		{
			obj->GetComponentRaw<Rendering2D>()->Render(this);
		}
	}

	window.display();
}

void RenderingSystem2D::PostIteration(float dt)
{
}

void RenderingSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	
}

void RenderingSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{

}

void RenderingSystem2D::AddCamera(Camera2D* cam)
{
	cam->m_id = m_cameraObjects.size();
	m_cameraObjects.push_back(cam->GetObject());
}

NAMESPACE_END
