struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler						: register(s0);

//always t3
Texture2D depthMap					: register(t3);


Texture2D godrays					: register(t4);
Texture2D scene						: register(t5);

cbuffer Info : register(b2)
{
	uint  type;
	float width;
	float height;
	float downScaleFactor;
};

float4 main(PS_INPUT input) : SV_TARGET0
{
	//float2 texelSize = float2(1.f / width, 1.f / height);
	//float2 downScaleTexelSize = texelSize * 2.0f;

	//float upSampledDepth = depthMap.Sample(defaultSampler, input.textCoord);

	//float3 color = 0.0f.xxx;
	//float totalWeight = 0.0f;

	//// Select the closest downscaled pixels.

	//int2 screenCoordinates = floor(input.textCoord * float2(width, height));

	//int xOffset = screenCoordinates.x % 2 == 0 ? -1 : 1;
	//int yOffset = screenCoordinates.y % 2 == 0 ? -1 : 1;

	//int2 offsets[] = { 
	//	int2(0, 0),
	//	int2(0, yOffset),
	//	int2(xOffset, 0),
	//	int2(xOffset, yOffset) 
	//};

	//for (int i = 0; i < 4; i++)
	//{
	//	float2 offset = offsets[i] * texelSize;
	//	float2 downScaleOffset = offsets[i] * downScaleTexelSize;

	//	float3 downscaledColor = godrays.Sample(defaultSampler, input.textCoord + downScaleOffset).xyz;

	//	float downscaledDepth = depthMap.Sample(defaultSampler, input.textCoord + downScaleOffset);

	//	float currentWeight = 1.0f;
	//	currentWeight *= max(0.0f, 1.0f - (0.05f) * abs(downscaledDepth - upSampledDepth));

	//	color += downscaledColor * currentWeight;
	//	totalWeight += currentWeight;

	//}

	//const float epsilon = 0.0001f;
	//float3 volumetricLight = color / (totalWeight + epsilon);

	float3 rayColor = godrays.Sample(defaultSampler, input.textCoord).xyz;
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).xyz;

	//return float4(saturate(volumetricLight + pixelColor), 1.0f);

	return float4(saturate(pixelColor + rayColor), 1.0f);
}