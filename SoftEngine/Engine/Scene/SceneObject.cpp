#include "SceneObject.h"

#include "Core/IObject.h"

#include "Engine/Controllers/Controller.h"
#include "Engine/Physics/PhysicsObject.h"

SceneObjectBase::~SceneObjectBase()
{
	static size_t i = 0;
	i++;

	std::cout << "\'" << TYPE_DESC[m_type] << "\' deleted (total object deleted: " << i << ").\n";

	switch (m_type)
	{
	case RENDERABLE_OBJECT:
		delete m_renderingObject.object;
		break;
	case CAMERA_OBJECT:
		delete m_renderingObject.camera;
		break;
	case LIGHT_OBJECT:
		break;
	default:
		break;
	}

	m_renderingObject.opaque = 0;

	delete m_controller;
	m_controller = 0;

	delete m_physicsObject;
	m_physicsObject = 0;
};