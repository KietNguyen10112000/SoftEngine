#pragma once

#include "Common/Base/SerializableDB.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

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
		SerializableDB::Get()->Register<AnimMeshRenderer>();

		// built-in script components
		SerializableDB::Get()->Register<FPPCameraScript>();

		// built-in animation components
		SerializableDB::Get()->Register<AnimSkeletalGameObject>();
		SerializableDB::Get()->Register<AnimatorSkeletalGameObject>();
		SerializableDB::Get()->Register<AnimatorSkeletalArray>();

	}
};

NAMESPACE_END