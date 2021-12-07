#pragma once

#include <Math/Math.h>

#include <vector>
#include <set>

#define _FIX_BORDER_POINT_LIGHT_OFFSET 0.01f

#define FORWARD_LIGHTING_START_RESOURCE_SLOT 4

struct LightSystemInfo
{
	Vec3 environmentAmbient;
	uint32_t numLight = 0;
};

//Use mega texture depth map 

//with directional or spot light, this is a single piece of of shadow map, has dimension dim
//with point light, this is 6 consecutive piece of shadow map to simulate a cube map, 
//the cube map order: right, left, up, down, front, back
struct ShadowAlloc
{
	//position
	Vec2 pos = { -1 ,-1 };
	//dimension
	Vec2 dim = { 64, 64 };

	inline ShadowAlloc() {};

	inline ShadowAlloc(Vec2 dimensions)
	{
		pos.x = -1;
		pos.y = -1;
		dim = dimensions;
	};
};

enum SHADOW_MAP_QUALITY
{
	//64x64
	ULTRA_LOW	= 0,
	//128x128
	VERY_LOW	= 1,
	//256x256
	LOW			= 2,
	//512x512
	MEDIUM		= 3,
	//1024x1024
	HIGH		= 4,
	//2048x2048
	VERY_HIGH	= 5,
	//2048x2048
	_2K			= 5,
	//4096x4096
	ULTRA_HIGH	= 6,
	//4096x4096
	_4K			= 6,
};

#define ShadowMap_BITMAP				Vec2(64, 64)
#define ShadowMap_ULTRA_LOW				Vec2(128, 128)
#define ShadowMap_LOW					Vec2(256, 256)
#define ShadowMap_MEDIUM				Vec2(512, 512)
#define ShadowMap_HIGH					Vec2(1024, 1024)
#define ShadowMap_2K					Vec2(2048, 2048)
#define ShadowMap_4K					Vec2(4096, 4096)

#define ShadowMap_NUM_CASCADE			4


enum LIGHT_TYPE
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
	CSM_DIRECTIONAL_LIGHT,
	UNKNOWN
};

struct Light
{
private:
	friend class LightSystem;
	friend class DX11LightSystem;

public:
	float spotAngle;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float power;

	Vec3 pos;
	Vec3 dir;
	Vec3 color;

private:
	//type of light
	uint32_t type;
	//index in m_lights, default value is non indexed
	uint32_t index = UINT32_MAX;
	//index in m_activeLights, default value is non indexed
	uint32_t activeIndex = UINT32_MAX;
	//index in m_activeShadowLight, default value is non indexed
	uint32_t activeShadowIndex = UINT32_MAX;

private:
	Light(
		uint32_t type,
		float spotAngle,
		float constantAttenuation,
		float linearAttenuation,
		float quadraticAttenuation,
		float power,
		Vec3 pos,
		Vec3 dir,
		Vec3 color
	);

public:
	~Light();

};

typedef Mat4x4* Shadow;

#define MAX_LIGHT 100'00
#define MAX_SHADOW_LIGHT 100

typedef unsigned int LightID;
typedef unsigned int ShadowID;
typedef unsigned int ShadowMapID;

#define LightID_Invalid UINT32_MAX
#define ShadowAllocID_Invalid UINT32_MAX

#define MIN_SHADOW_BLOCK_SIZE 64

#define To1DIndex(i, j, w, h) (j) * (w) + i

class LightSystem
{
public:
	inline const static Vec2 shadowMapQuality[] = {
		Vec2(64, 64),
		Vec2(128, 128),
		Vec2(256, 256),
		Vec2(512, 512),
		Vec2(1024, 1024),
		Vec2(2048, 2048),
		Vec2(4096, 4096),
	};

protected:
	uint32_t m_lightAllocCounter = 0;
	std::set<uint32_t, std::less<uint32_t>> m_lightFreeSpaces;
	//all lights in space
	std::vector<Light> m_lights;

	struct ExtraDataDirLight
	{
		//Mat4x4 vpMat;
		ShadowAlloc alloc = {};
		float farPlane;
		float nearPlane;
		float w;
		float h;
		float fov;

		//uint16_t shadowIndex = UINT16_MAX;
	};

	struct ExtraDataPointLight
	{
		//Mat4x4 vpMat[6];
		ShadowAlloc alloc[6] = {};
		float farPlane;
		float nearPlane;

		//uint16_t shadowIndex = UINT16_MAX;
	};

	//index is shadowID
	std::vector<void*> m_lightExtraData;

	//uint32_t m_shadowLightAllocCounter = 0;
	//std::set<uint32_t, std::less<uint32_t>> m_shadowLightFreeSpaces;
	////all viewProj matrix of the lights make shadow in space
	

	//the lights will be render to this frame, maximum = MAX_LIGHT
	//will send to structured buffer in GPU
	//bool m_updateActiveLight = false;
	//std::vector<uint16_t> m_activeLightFreeSpaces;
	std::vector<Light> m_activeLights;

	struct LightShadow
	{
		//if dir or spot light only uvOffset[0] used, 
		Vec2 uvOffset[6] = {};
		Vec2 texelDim;

		Mat4x4 viewProj[6] = {};
	};
	//will send to structured buffer in GPU
	std::vector<uint16_t> m_activeLightShadowFreeSpaces;
	std::vector<LightShadow> m_activeLightShadow;

	uint32_t m_shadowMapSize = 0;

	using Iter = std::set<uint32_t, std::less<uint32_t>>::iterator;

	std::set<uint32_t, std::less<uint32_t>> m_shadowMapFreeIndex;

//#ifdef _DEBUG
	std::vector<std::vector<uint32_t>> m_shadowMap;
//#else
//	uint32_t** m_shadowMap;
//#endif

	//do update to GPU
	std::set<LightID> m_updateActiveLights;

	//do update to GPU
	std::set<uint32_t> m_updateActiveShadowLights;


public:
	LightSystem(uint32_t dim);
	virtual ~LightSystem();

protected:
	ShadowAlloc GetFreeSpaceAlloc(Vec2 dim, LightID id);

	void FreeSpaceAlloc(const ShadowAlloc& alloc, LightID id);

	void UpdatePointLight(ExtraDataPointLight* p, Light& light, Mat4x4* vpMat, float near, float far);
	void UpdateDirLight(ExtraDataDirLight* p, Light& light, Mat4x4* vpMat, float fov, float near, float far, float w, float h);

	void DoAddShadow(Light& light, LightShadow& shadow);

public:
	//new light to space
	LightID NewLight(
		uint32_t type,
		float spotAngle,
		float constantAttenuation,
		float linearAttenuation,
		float quadraticAttenuation,
		float power,
		Vec3 pos,
		Vec3 dir,
		Vec3 color
	);

	//delete from space
	void DeleteLight(LightID id);
	void DeleteAllLight();

	//to get info, modify, ...
	inline Light& GetLight(LightID id) { if (id < m_lights.size()) return m_lights[id]; return m_lights[0]; };

	//if modify, call update
	void UpdateLight(LightID id);

	//add light to render, this light will be render to current frame
	void AddLight(LightID id);
	//remove light, this light will not be render to current frame, just remove, not delete
	void RemoveLight(LightID id);
	
	//clear all light in frame
	void ClearLight();


public:
	//add shadow to render, this shadow will be render to current frame
	void AddShadow(LightID id, Vec2 shadowMapDimentions, float fov = -1, float near = 0.5f, float far = 250.0f, float w = 100, float h = 100);
	void AddShadow(LightID id, SHADOW_MAP_QUALITY quality, float fov = -1, float near = 0.5f, float far = 250.0f, float w = 100, float h = 100);

	void AddShadowEx(LightID id, Vec2* shadowMapDimentions, uint32_t numShadow, uint32_t numArgs, void** args);

	void ReallocShadowMap(LightID id, Vec2 newShadowMapDims);
	void RemoveShadow(LightID id);

	//return view * projection matrix that present shadow
	Mat4x4* GetShadow(LightID id);

	//use for point light and spot light
	void UpdateShadow(LightID id, float fov = -1, float near = -1, float far = -1, float w = -1, float h = -1);

	void ForceUpdateShadow(LightID id);
	
public:
	inline virtual bool BeginShadow(LightID id) { return false; };
	inline virtual void EndShadow(LightID id) {};

public:
	inline virtual void Update() {};

	void Log();

};