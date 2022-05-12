#pragma once

#ifdef _DEBUG

#ifdef _WIN32
#pragma comment(lib, "Bullet3Collision_Debug.lib")
#pragma comment(lib, "Bullet3Common_Debug.lib")
#pragma comment(lib, "Bullet3Dynamics_Debug.lib")
#pragma comment(lib, "Bullet3Geometry_Debug.lib")
#pragma comment(lib, "BulletCollision_Debug.lib")
#pragma comment(lib, "BulletDynamics_Debug.lib")
#pragma comment(lib, "BulletInverseDynamicsUtils_Debug.lib")
#pragma comment(lib, "BulletInverseDynamics_Debug.lib")
#pragma comment(lib, "BulletRobotics_Debug.lib")
#pragma comment(lib, "BulletSoftBody_Debug.lib")
#pragma comment(lib, "ConvexDecomposition_Debug.lib")
#pragma comment(lib, "LinearMath_Debug.lib")
#endif // _WIN32

#else

#ifdef _WIN32
#pragma comment(lib, "Bullet3Collision.lib")
#pragma comment(lib, "Bullet3Common.lib")
#pragma comment(lib, "Bullet3Dynamics.lib")
#pragma comment(lib, "Bullet3Geometry.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletInverseDynamicsUtils.lib")
#pragma comment(lib, "BulletInverseDynamics.lib")
#pragma comment(lib, "BulletRobotics.lib")
#pragma comment(lib, "BulletSoftBody.lib")
#pragma comment(lib, "ConvexDecomposition.lib")
#pragma comment(lib, "LinearMath.lib")
#endif

#endif // _DEBUG
