#include "../Include/Light.hlsli"

struct PS_INPUT
{
	float4 pos							: SV_POSITION;
	float4 position						: WORLD_POSITION;
	uint cubeFaceIndex					: SV_RenderTargetArrayIndex;
};

StructuredBuffer<Light> lights			: register(t3);

cbuffer LightInfo : register(b5)
{
	uint index;
	float farPlane;
	float2 padding;
};

float main(PS_INPUT input) : SV_Depth
{
	//float lightDistance = distance(input.position.xyz, lights[index].pos);
	//lightDistance = lightDistance / farPlane;
	//return lightDistance;

	return input.position.z / input.position.w;
}