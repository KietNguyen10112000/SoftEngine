#pragma once

#include "Common/Base/SerializableDB.h"
#include "Common/Math/Function1D.h"

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
#include "MainSystem/Animation/AnimLayer/AnimMixLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimTransitLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimJointLayer.h"

#include "MainSystem/Physics/Components/RigidBodyStatic.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"
#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/Materials/PhysicsMaterial.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeBox.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"
#include "MainSystem/Physics/Shapes/PhysicsShapePlane.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeSphere.h"
#include "MainSystem/Physics/Joints/RevoluteJoint.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "Resources/AnimModel.h"
#include "Resources/Texture2D.h"
#include "Resources/Model3D.h"
#include "Resources/AnimMotion.h"

NAMESPACE_BEGIN

// initialize meta data for built-in main components
class SerializableList
{
public:
	inline static void Initialize()
	{
		SerializableDB::Get()->Register<Scene>();
		SerializableDB::Get()->Register<GameObject>();


		// common math
		{
			SerializableDB::Get()->Register<FunctionLinear1D>();
			SerializableDB::Get()->Register<FunctionQuadratic1D>();
		}


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
			SerializableDB::Get()->Register<AnimTransitLayer>();
			SerializableDB::Get()->Register<AnimBlendLayer>();
			SerializableDB::Get()->Register<AnimMixLayer>();
			SerializableDB::Get()->Register<AnimJointLayer>();
		}


		// built-in physics components
		{
			SerializableDB::Get()->Register<RigidBodyStatic>();
			SerializableDB::Get()->Register<RigidBodyDynamic>();
			SerializableDB::Get()->Register<CharacterControllerCapsule>();

			SerializableDB::Get()->Register<PhysicsMaterial>();
			SerializableDB::Get()->Register<PhysicsShapeBox>();
			SerializableDB::Get()->Register<PhysicsShapeCapsule>();
			SerializableDB::Get()->Register<PhysicsShapePlane>();
			SerializableDB::Get()->Register<PhysicsShapeSphere>();
			SerializableDB::Get()->Register<RevoluteJoint>();
		}


		{
			SerializableDB::Get()->RegisterResource<AnimModel>();
			SerializableDB::Get()->RegisterResource<AnimMotion>();
			SerializableDB::Get()->RegisterResource<Model3D>();
			SerializableDB::Get()->RegisterResource<Texture2D>();
		}
	}
};

NAMESPACE_END