struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler						: register(s0);

//always t3
Texture2D depthMap					: register(t3);


Texture2D scene						: register(t4);
Texture2D shadowColor				: register(t5);


float4 main(PS_INPUT input) : SV_TARGET0
{
	//float2 textCoord = input.textCoord * 1.005f;
	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);

	if (pixelDepth == 1.0f)
	{
		discard;
	}

	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).xyz;
	float4 shadowC = shadowColor.Sample(defaultSampler, input.textCoord).xyzw;

	return float4(pixelColor * shadowC, 1.0f);

	//if (shadowC.w != 1) return float4(1, 0, 0, 1.0f);

	//int2 screenCoordinates = floor(input.textCoord * float2(1280, 960));
	//float2 texelSize = float2(1 / 1280.0f, 1 / 960.0f);

	//int xOffset = screenCoordinates.x % 2 == 0 ? -1 : 1;
	//int yOffset = screenCoordinates.y % 2 == 0 ? -1 : 1;

	//int2 offsets[] = { 
	//	int2(0, 0),
	//	int2(0, yOffset),
	//	int2(xOffset, 0),
	//	int2(xOffset, yOffset) 
	//};

	//float3 color = 0.0f.xxx;
	//float totalWeight = 0;
	//for (int i = 0; i < 4; i++)
	//{
	//	//float2 offset = offsets[i] * texelSize;
	//	float2 offset = offsets[i] * texelSize;

	//	float3 cColor = scene.Sample(defaultSampler, input.textCoord).xyz;
	//	float3 shadowC = shadowColor.Sample(defaultSampler, input.textCoord).xyz;

	//	float cDepth = depthMap.Sample(defaultSampler, input.textCoord + offset).r;

	//	float currentWeight = 1.0f;
	//	currentWeight *= max(0.0f, 1.0f - 0.05f * abs(cDepth - pixelDepth));

	//	color += (cColor * shadowC) * currentWeight;
	//	totalWeight += currentWeight;
	//}
	//const float epsilon = 0.0001f;
	//color /= (totalWeight + epsilon);

	//return float4(color, 1.0f);
}