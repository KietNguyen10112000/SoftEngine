#ifndef __cplusplus
	#define Vec3 float3
	#define Vec4 float4
	#define Mat4 row_major float4x4
	#define uint32_t uint
#endif

#include "Common.hlsli"
#include "../../../../../../MainSystem/Rendering/BuiltinConstantBuffers.inl"

struct Light
{
	float spotAngle;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float power;

	float3 pos;
	float3 dir;
	float3 color;

	uint type;
	uint index; //index in structured buffer
	uint activeIndex;
	uint activeShadowIndex;
};

const static Light g_light = {
	0,
	0,
	0,
	0,
	1,

	float3(0,0,0),
	float3(-0.57735026919f,-0.57735026919f,-0.57735026919f),
	float3(1,1,1),

	0,
	0,
	0,
	0
};