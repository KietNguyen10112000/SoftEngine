#pragma once

#include "Common/Base/SerializableDB.h"

#include "MainSystem/Rendering/Components/Camera.h"
#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"
#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"
#include "MainSystem/Scripting/Components/TPPCameraScript.h"

//#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"
//#include "MainSystem/Animation/Components/AnimatorSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"

#include "MainSystem/Physics/Components/RigidBodyStatic.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

// initialize meta data for built-in main components
class SerializableList
{
public:
	inline static void Initialize()
	{
		SerializableDB::Get()->Register<Scene>();
		SerializableDB::Get()->Register<GameObject>();


		// built-in rendering components
		{
			SerializableDB::Get()->Register<Camera>();
			SerializableDB::Get()->Register<CameraTPP>();
			SerializableDB::Get()->Register<MeshBasicRenderer>();
			SerializableDB::Get()->Register<AnimMeshRenderer>();
			SerializableDB::Get()->Register<AnimModelStaticMeshRenderer>();

			SerializableDB::Get()->Register<AnimModel::AnimMeshRenderingBuffer>();
		}


		// built-in script components
		{
			SerializableDB::Get()->Register<FPPCameraScript>();
			SerializableDB::Get()->Register<TPPCameraScript>();
		}


		// built-in animation components
		{
			//SerializableDB::Get()->Register<AnimSkeletalGameObject>();
			//SerializableDB::Get()->Register<AnimatorSkeletalGameObject>();
			SerializableDB::Get()->Register<AnimatorSkeletalArray>();

			// animation layers
			SerializableDB::Get()->Register<AnimPlayerLayer>();
			SerializableDB::Get()->Register<AnimBlendLayer>();
		}


		// built-in physics components
		{
			SerializableDB::Get()->Register<RigidBodyStatic>();
			SerializableDB::Get()->Register<RigidBodyDynamic>();
			SerializableDB::Get()->Register<CharacterControllerCapsule>();
		}
	}
};

NAMESPACE_END