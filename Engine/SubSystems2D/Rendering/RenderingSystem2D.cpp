#include "RenderingSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"

#include "Components2D/Rendering/Camera2D.h"

#include "Graphics2D/Graphics2D.h"

#include <iostream>

#include "imgui.h"
#include "imgui-SFML.h"


NAMESPACE_BEGIN

RenderingSystem2D::RenderingSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	m_renderList.reserve(64 * KB);
	m_querySession = scene->NewQuerySession();
}

RenderingSystem2D::~RenderingSystem2D()
{
	m_scene->DeleteQuerySession(m_querySession);
}

void RenderingSystem2D::PrevIteration(float dt)
{
}

//#define RENDER_OBJECT(obj)										\
//GameObject2D::PostTraversal(obj, [&](GameObject2D* o)			\
//{																\
//	auto r = o->GetComponentRaw<Rendering2D>();					\
//	if (r)														\
//	{															\
//		r->Render(this);										\
//	}															\
//});
//
//#define Z_ORDER(obj) (obj)->GetComponentRaw<Rendering2D>()->m_zOrder

void RenderingSystem2D::Iteration(float dt)
{
	if (m_refreshGhostObject)
	{
		std::sort(m_ghostObjects.begin(), m_ghostObjects.end(),
			[](GameObject2D* o1, GameObject2D* o2)
			{
				return o1->GetComponentRaw<Rendering2D>()->m_zOrder 
					< o2->GetComponentRaw<Rendering2D>()->m_zOrder;
			}
		);

		m_refreshGhostObject = false;
	}
	
	m_renderList.clear();
	for (auto& obj : m_ghostObjects)
	{
		m_renderList.push_back(obj->GetComponentRaw<Rendering2D>());
	}

	if (!Graphics2D::Get()) return;

	auto& window = Graphics2D::Get()->m_window;
	window.clear();

	m_bindedWindow = &window;

	if (m_cameraObjects.size() != 0)
	{
		auto camObj = m_cameraObjects[0];
		auto cam = m_cameraObjects[0]->GetComponentRaw<Camera2D>();
		auto& rect = cam->Rect();

		m_bindedCamera = cam;

		sf::View view1;
		auto pos = cam->GetCenter();
		AARect view = cam->GetView();

		view1.setCenter(reinterpret_cast<sf::Vector2f&>(pos));
		view1.setSize(reinterpret_cast<const sf::Vector2f&>(rect.GetDimensions()));
		window.setView(view1);

		m_querySession->Clear();
		m_scene->AABBStaticQueryAARect(view, m_querySession);
		m_scene->AABBDynamicQueryAARect(view, m_querySession);
		
		for (auto& obj : *m_querySession)
		{
			GameObject2D::PrevTraversal(obj,
				[&](GameObject2D* o) 
				{
					auto r = o->GetComponentRaw<Rendering2D>();
					if (r)
					{
						m_renderList.push_back(r);
					}
				}
			);
		}
		
	}

	std::sort(m_renderList.begin(), m_renderList.end(),
		[](Rendering2D* r1, Rendering2D* r2)
		{
			return r1->m_zOrder < r2->m_zOrder;
		}
	);

	for (auto& r : m_renderList)
	{
		if (r->m_visible)
			r->Render(this);
	}

	auto size = window.getSize();
	window.setView(sf::View({ (float)size.x / 2.0f,(float)size.y / 2.0f }, { (float)size.x, (float)size.y }));

	static sf::Clock deltaClock;

	ImGui::SFML::Update(reinterpret_cast<sf::RenderWindow&>(window), deltaClock.restart());
	/*ImGui::ShowDemoWindow();
	ImGui::Begin(u8"Nguyễn Hữu Kiệt");
	ImGui::Button("Look at this pretty button");
	ImGui::End();*/

	m_scene->GetScriptSystem()->ForEachOnGUIScripts(
		[](Script2D* script)
		{
			script->OnGUI();
		}
	);

	ImGui::SFML::Render(window);

	window.display();
}

void RenderingSystem2D::PostIteration(float dt)
{
}

void RenderingSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	auto rd = (Rendering2D*)comp;
	auto obj = comp->GetObject();
	if (obj->m_type == GameObject2D::GHOST && obj->IsRoot())
	{
		rd->m_id = m_ghostObjects.size();
		m_ghostObjects.push_back(obj);

		m_refreshGhostObject = true;
	}
}

void RenderingSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	auto rd = (Rendering2D*)comp;
	auto obj = comp->GetObject();
	if (obj->m_type == GameObject2D::GHOST && obj->IsRoot())
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_ghostObjects, rd->m_id, back->GetComponentRaw<Rendering2D>()->m_id);

		m_refreshGhostObject = true;
	}
}

void RenderingSystem2D::AddCamera(Camera2D* cam)
{
	cam->m_id = m_cameraObjects.size();
	m_cameraObjects.push_back(cam->GetObject());
}

NAMESPACE_END
