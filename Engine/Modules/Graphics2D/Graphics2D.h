#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

#include "SFML/Graphics.hpp"

NAMESPACE_BEGIN

using DebugGraphics2D = void;
using GraphicsCommandList = void;
class RenderingSystem;
class Camera2D;

class API Graphics2D : public Singleton<Graphics2D>
{
public:
	DebugGraphics2D* m_debugGraphics = nullptr;

	RenderingSystem* m_bindedRdSys = nullptr;

	sf::RenderWindow m_window;

public:
	virtual ~Graphics2D() {};

public:
	static int Initilize(sf::Window*& output, int width, int height);
	static void Finalize();

public:
	void BeginCamera(Camera2D* camera);
	void EndCamera(Camera2D* camera);

public:
	inline auto GetDebugGraphics()
	{
		return m_debugGraphics;
	}

	inline void Bind(RenderingSystem* sys)
	{
		m_bindedRdSys = sys;
	}

	inline auto GetRenderingSystem()
	{
		return m_bindedRdSys;
	}

	inline auto& GetSFWindow()
	{
		return m_window;
	}

};

NAMESPACE_END