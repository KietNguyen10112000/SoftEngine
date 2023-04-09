#ifndef CPP
	#define Vec3 float3
	#define Vec4 float4
	#define Mat4 row_major float4x4
	#define uint32_t uint
#endif

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