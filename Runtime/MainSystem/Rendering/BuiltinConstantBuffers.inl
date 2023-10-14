struct SceneData
{
	// time since engine startup
	float t;

	// delta time
	float dt;

	float padding[2];

	uint32_t x;
	uint32_t xx;
	uint32_t xxx;
	uint32_t xxxx;
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

	uint32_t directionLightsCount;
	uint32_t spotLightsCount;
	uint32_t pointLightsCount;
	uint32_t x;
};

struct ObjectData
{
	Mat4 transform;
};