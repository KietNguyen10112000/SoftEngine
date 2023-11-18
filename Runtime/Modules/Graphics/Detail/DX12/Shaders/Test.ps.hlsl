//#define D3DCOMPILE_DEBUG 1
#include "Common/TypeDef.hlsli"

struct PS_INPUT
{
	float4 svposition	: SV_POSITION;
	float3 position		: POSITION;
	float3x3 TBN		: TBN_MATRIX;
	float2 textCoord	: TEXTCOORD;
};

SamplerState	defaultSampler			: register(s0);

Texture2D		color					: register(t0, space1);

cbuffer CameraCBuffer : register(b0, SPACE_PS)
{
	CameraData Camera;
};

struct Pixel
{
	float3 pixelPos;
	float3 normal;
	float  specular;
	float  shininess;
};

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

float DoSpecular(float3 lightDir, float3 normal, float3 viewDir)
{
	//Phong
	/*float3 reflectDir = normalize(reflect(-lightDir, normal));
	return max(dot(viewDir, reflectDir), 0);*/

	//Blinn
	float3 halfwayDir = -normalize(lightDir + viewDir);
	return max(dot(halfwayDir, normal), 0);
}

LightingResult DoDirectionalLight(Light light, Pixel input, float3 viewPoint)
{
	LightingResult result;

	float3 lightDir = light.dir.xyz;

	float diff = max(0, dot(input.normal, -lightDir));

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * light.color;
	result.specular = spec * light.color * input.specular;

	return result;
}

float4 main(PS_INPUT input) : SV_TARGET
{
	Pixel px;
	px.pixelPos = input.position;
	px.normal = normalize(input.TBN[2]);
	px.specular = 0.5f;
	px.shininess = 64;

	LightingResult lastResult = DoDirectionalLight(g_light, px, Camera.transform._m30_m31_m32);

	float4 pixelColor = float4(color.Sample(defaultSampler, input.textCoord).rgb, 1.0f);
	pixelColor *= float4(saturate((lastResult.diffuse + lastResult.specular)), 1.0f);

	return pixelColor;

	//return float4(Camera.transform._m30_m31_m32, 1.0f);
}