#include "Graphics2D.h"

#include "Core/Memory/Memory.h"

//#include "API/API.h"

#include "Platform/Platform.h"

#include "Network/TCPConnector.h"

NAMESPACE_BEGIN

int Graphics2D::Initilize(sf::Window*& output, int width, int height)
{
	Graphics2D::s_instance.reset(rheap::New<Graphics2D>());
	auto graphics = Graphics2D::Get();

	output = &graphics->m_window;

	output->create(sf::VideoMode(width, height), "SoftEngine");

	output->setVerticalSyncEnabled(true);

	return 0;
}

void Graphics2D::Finalize()
{
	auto g = Graphics2D::s_instance.release();
	rheap::Delete(g);
}

void Graphics2D::BeginCamera(Camera2D* camera)
{
}

void Graphics2D::EndCamera(Camera2D* camera)
{
}

NAMESPACE_END