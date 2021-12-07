#include "../../../Include/Light.hlsli"

//first 20 lights must be the importance lights
#define MAX_FORWARD_LIGHTS 20

struct PS_INPUT
{
	float4 pos				: SV_POSITION;
	float4 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3x3 TBN			: TBN_MATRIX;

	float depth				: DEPTH;

	float  alpha			: ALPHA;
};

//Texture2D diffuse			: register(t4);
Texture2D diffuseMap		: register(t4);
Texture2D normalMap			: register(t5);
Texture2D metallicMap		: register(t6);
Texture2D roughnessMap		: register(t7);
Texture2D aoMap				: register(t8);


cbuffer Material			: register(b2)
{
	float3 ambient;
	float specular;
	float3 diffuse;
	float shininess;
};

cbuffer ShadingTypeInfo			: register(b3)
{
	int shadingType;
	float3 padding;
};


float4 NormalShading(PS_INPUT input)
{
	NormalShadingPixel curPixel = (NormalShadingPixel)0;

	curPixel.color = diffuseMap.Sample(defaultSampler, input.textCoord);

	if (input.alpha != 0) curPixel.color.w = input.alpha;

	curPixel.color.xyz = (diffuse + ambient) * curPixel.color.xyz;

	curPixel.pixelPos = input.position.xyz;
	curPixel.specular = specular;

	curPixel.normal = normalize(input.TBN[2]);
	curPixel.shininess = shininess;

	curPixel.depth = input.depth;

	return DoNormalShading(curPixel);
}

float4 NormalMapShading(PS_INPUT input)
{
	NormalShadingPixel curPixel = (NormalShadingPixel)0;

	curPixel.color = diffuseMap.Sample(defaultSampler, input.textCoord);

	if (input.alpha != 0) curPixel.color.w = input.alpha;

	curPixel.color.xyz = (diffuse + ambient) * curPixel.color.xyz;

	curPixel.pixelPos = input.position.xyz;
	curPixel.specular = specular;

	float3 norm = normalMap.Sample(defaultSampler, input.textCoord);
	norm = norm * 2 - 1;
	norm = mul(norm, input.TBN);
	curPixel.normal = normalize(norm);
	//curPixel.normal = normalize(input.TBN[2]);

	curPixel.shininess = shininess;

	curPixel.depth = input.depth;

	return DoNormalShading(curPixel);
}

// ----------------------------------------------------------------------------------------------------------------------------

float4 PBRShading(PS_INPUT input)
{
	float4 pixelColor = diffuseMap.Sample(defaultSampler, input.textCoord);
	float3 ns = normalMap.Sample(defaultSampler, input.textCoord);

	PBRShadingPixel pixel = (PBRShadingPixel)0;

	pixel.pos = input.position.xyz;
	pixel.normal = normalize(ns);
	pixel.color = float4(pow(pixelColor.xyz, float3(2.2, 2.2, 2.2)), pixelColor.w);

	pixel.metallic = metallicMap.Sample(defaultSampler, input.textCoord).r;
	pixel.roughness = roughnessMap.Sample(defaultSampler, input.textCoord).r;
	pixel.ao = aoMap.Sample(defaultSampler, input.textCoord).r;

	pixel.depth = input.depth;

	return DoPBRShading(pixel);
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);
	
	if (pixelDepth > 0.9999f) return float4(0, 0, 0, 0);

	if (shadingType == 0) return PBRShading(input);

	if (shadingType == 1) return NormalMapShading(input);

	return NormalShading(input);
}