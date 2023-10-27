#pragma once

#include "MainComponentDB.h"

#include "Rendering/Components/Camera.h"
#include "Rendering/Components/Model3DBasicRenderer.h"

#include "Scripting/Components/FPPCameraScript.h"

NAMESPACE_BEGIN

// initialize meta data for built-in main components
class MainComponentList
{
public:
	inline static void Initialize()
	{
		MainComponentDB::Get()->RegisterComponent<Camera>();
		MainComponentDB::Get()->RegisterComponent<Model3DBasicRenderer>();

		MainComponentDB::Get()->RegisterComponent<FPPCameraScript>();
	}
};

NAMESPACE_END