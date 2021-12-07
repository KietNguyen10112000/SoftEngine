struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler				: register(s0);

Texture2D depthMap						: register(t3);
Texture2D scene							: register(t4);


cbuffer Info : register(b2)
{
	uint  type;
	float width;
	float height;
	float padding;

	//row_major float4x4 padding2;
};

//const static float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
const static float weight[4] = { 0.175240144, 0.165770069, 0.140321344, 0.106288522 };
//const static float weight[4] = { 0.00159151943, 0.001518361155, 0.00156003464, 0.49612584449 };
#define TOTAL_WEIGHT 4

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).xyz * weight[0];

	float2 texelSize = float2(1.5f / width, 1.5f / height);

	if (type == 0)
	{
		//horizontal
		[unroll] for (int i = 1; i < TOTAL_WEIGHT; i++)
		{
			pixelColor += scene.Sample(defaultSampler, input.textCoord + float2(texelSize.x * i, 0.0f)).xyz * weight[i];
			pixelColor += scene.Sample(defaultSampler, input.textCoord - float2(texelSize.x * i, 0.0f)).xyz * weight[i];
		}
	}
	else
	{
		//vertical
		[unroll] for (int i = 1; i < TOTAL_WEIGHT; i++)
		{
			pixelColor += scene.Sample(defaultSampler, input.textCoord + float2(0.0f, texelSize.y * i)).xyz * weight[i];
			pixelColor += scene.Sample(defaultSampler, input.textCoord - float2(0.0f, texelSize.y * i)).xyz * weight[i];
		}
	}

	
	return float4(pixelColor, 1.0f);
}