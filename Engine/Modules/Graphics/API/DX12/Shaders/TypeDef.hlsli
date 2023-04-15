#ifdef CPP
#pragma once
#endif // CPP

#ifndef CPP
	#define Vec3 float3
	#define Vec4 float4
	#define Mat4 row_major float4x4
	#define uint32_t uint

#define PI 3.14159265359f
#endif

#define MAX_DIR_LIGHT	16
#define MAX_POINT_LIGHT 256
#define MAX_SPOT_LIGHT	256

//struct LightControlStruct
//{
//	uint32_t index;
//	uint32_t activeShadowIndex;
//};

struct LightDir
{
	float power;
	float padding;

	Vec3 dir;
	Vec3 color;

	uint32_t index;
	uint32_t shadowIndex;
};

struct LightPoint
{
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float power;

	Vec3 pos;
	Vec3 color;

	uint32_t index;
	uint32_t shadowIndex;
};

struct LightSpot
{
	float spotAngle;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float power;

	Vec3 pos;
	Vec3 dir;
	Vec3 color;

	uint32_t index;
	uint32_t shadowIndex;
};

struct SceneData
{
	// time since engine startup
	float t;

	// delta time
	float dt;

	float padding[2];

	uint32_t directionLightsCount;
	uint32_t spotLightsCount;
	uint32_t pointLightsCount;

	uint32_t padding2;

	LightDir	dirLights	[MAX_DIR_LIGHT];
	LightPoint	pointLights	[MAX_POINT_LIGHT];
	LightSpot	spotLights	[MAX_SPOT_LIGHT];
};

struct CameraData
{
	Mat4 transform;

	// view is inversed of transform
	Mat4 view;
	Mat4 proj;

	// view * proj
	Mat4 vp;
	Mat4 inversedVp;

	float fovAngleY;
	float aspectRatio;
	float nearZ;
	float farZ;
};

struct ObjectData
{
	Mat4 transform;
};

struct StaticMeshData
{
	Mat4 transform;
};