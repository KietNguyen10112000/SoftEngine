#pragma once

#include "Common/Base/SerializableDB.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

// initialize meta data for built-in main components
class SerializableList
{
public:
	inline static void Initialize()
	{
		SerializableDB::Get()->Register<GameObject>();

		// built-in rendering components
		SerializableDB::Get()->Register<Camera>();
		SerializableDB::Get()->Register<MeshBasicRenderer>();

		// built-in script components
		SerializableDB::Get()->Register<FPPCameraScript>();
	}
};

NAMESPACE_END