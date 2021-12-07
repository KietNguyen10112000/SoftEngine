#include "../Include/Light.hlsli"

#include "../Include/LightingResource.hlsli"

struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

float4 NormalShading(PS_INPUT input, float4 posAndSpec)
{
	NormalShadingPixel curPixel = (NormalShadingPixel)0;

	//float4 pixelColor = float4(color.Sample(defaultSampler, input.textCoord).xyz, 1);
	float4 pixelColor = color.Sample(defaultSampler, input.textCoord);

	//pixel position in world space
	//float4 posAndSpec = positionAndSpec.Sample(defaultSampler, input.textCoord);

	curPixel.pixelPos = posAndSpec.xyz;
	curPixel.specular = posAndSpec.w;

	float4 ns = normAndShininess.Sample(defaultSampler, input.textCoord);

	if (all(ns.xyz == 0))
	{
		return pixelColor;
	}

	curPixel.normal = normalize(ns.xyz);
	curPixel.shininess = ns.w;

	curPixel.color = pixelColor;

	curPixel.depth = depthMap.Sample(defaultSampler, input.textCoord);

	//float dz = abs(curPixel.pixelPos.z - viewPoint.z);
	//float dx = abs(curPixel.pixelPos.x - viewPoint.x);
	////float pdy = abs(viewPoint.y) * 0.3f;


	//if (dx < 50 && dz < 50) curPixel.color.xyz = (curPixel.color.xyz + float3(0, 1, 0)) / 2;
	//else if (dx < 100 && dz < 100) curPixel.color.xyz = (curPixel.color.xyz + float3(1, 0, 0)) / 2;

	return DoNormalShading(curPixel);
}

// ----------------------------------------------------------------------------------------------------------------------------

float4 PBRShading(PS_INPUT input, float4 posAndSpec)
{
	//float4 ret = (float4)0;
	float4 mra = metallicRoughnessAO.Sample(defaultSampler, input.textCoord);
	//float4 posAndSpec = positionAndSpec.Sample(defaultSampler, input.textCoord);
	float4 pixelColor = color.Sample(defaultSampler, input.textCoord);
	float4 ns = normAndShininess.Sample(defaultSampler, input.textCoord);

	PBRShadingPixel pixel = (PBRShadingPixel)0;

	pixel.pos = posAndSpec.xyz;
	pixel.normal = normalize(ns.xyz);
	pixel.color = float4(pow(pixelColor.xyz, float3(2.2, 2.2, 2.2)), pixelColor.w);
	//pixel.color = pixelColor;
	pixel.metallic = mra.x;
	pixel.roughness = mra.y;
	pixel.ao = mra.z;

	pixel.depth = depthMap.Sample(defaultSampler, input.textCoord);

	return DoPBRShading(pixel);
}

float4 main(PS_INPUT input) : SV_TARGET
{
	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);

	if (pixelDepth == 1) discard;

	float4 posAndSpec = positionAndSpec.Sample(defaultSampler, input.textCoord);

	if (posAndSpec.w == -1)
	{	
		return PBRShading(input, posAndSpec);
		//return float4(1, 0, 0, 1);
	}

	return NormalShading(input, posAndSpec);
}