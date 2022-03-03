// SoftEngine.cpp : Defines the entry point for the application.
//

#include "SoftEngine.h"

//int main()
//{
//    FbxManager* lSdkManager = NULL;
//    FbxScene* lScene = NULL;
//
//    // Prepare the FBX SDK.
//    InitializeSdkObjects(lSdkManager, lScene);
//
//    if (!LoadScene(lSdkManager, lScene, "D:/KEngine/ResourceFile/model/character/Character Running.fbx"))
//    {
//        std::cout << "Error.\n";
//    }
//
//    FbxNode* rootNode = lScene->GetRootNode();
//
//    rootNode->GetChild(0)->GetNodeAttribute()->GetAttributeType();
//
//    DestroySdkObjects(lSdkManager, 1);
//
//	return 0;
//}

//#define DX11_RENDERER 1
//#define WIN32_WINDOW 1

#include "Engine/Engine.h"
#include <vector>
#include <Math/Collision.h>
//#include <Component/Frustum.h>


//#include "../DX11/LightSystem_v2.h"

//frustum sphere
//void CalBoundingSphere(std::vector<Vec3>& corners, Vec3* outCenter, float* outRadius)
//{
//	//0-----1
//	//|     |
//	//3-----2
//	auto* farPlane = &corners[4];
//	auto* nearPlane = &corners[0];
//	float w = (nearPlane[0] - nearPlane[1]).Length();
//	float h = (nearPlane[1] - nearPlane[2]).Length();
//
//	float n = nearPlane[0].z;
//	float f = farPlane[0].z;
//
//	auto v1 = (nearPlane[0] + nearPlane[3]) / 2.0f;
//	auto v2 = (nearPlane[1] + nearPlane[2]) / 2.0f;
//	float fovX = GetAngleBetween(v1, v2);
//
//
//	float k = std::sqrt(1 + (h / w) * (h / w)) * std::tan(fovX / 2.0f);
//
//	if (k * k >= ((f - n) / (f + n)))
//	{
//		*outCenter = { 0,0,-f };
//		*outRadius = f * k;
//	}
//	else
//	{
//		*outCenter = { 0,0,-0.5f * (f + n) * (1 + k * k) };
//		*outRadius = 0.5f * std::sqrt(
//			(f - n) * (f - n) +
//			2 * (f * f + n * n) * k * k +
//			(f + n) * (f + n) * k * k * k * k
//		);
//	}
//}

int main()
{
#if defined(_DEBUG) || defined(LOCAL_RELEASE)
	Engine* engine = new Engine(L"SoftEngine", 1280, 960);
#else
	Engine* engine = new Engine(L"SoftEngine", 0, 0);
#endif // _DEBUG

	engine->Loop();

	delete engine;

	return 0;
}


//#ifdef _DEBUG
//	Engine* engine = new Engine(L"SoftEngine", 1280, 960);
//#else
//	Engine* engine = new Engine(L"SoftEngine", 0, 0);
//#endif // _DEBUG
//
//	engine->Loop();
//
//	delete engine;