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

const static float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
//const static float weight[4] = { 0.175240144, 0.165770069, 0.140321344, 0.106288522 };
//const static float weight[4] = { 0.03071, 0.02905, 0.02459, 0.01863 };
//const static float weight[4] = { 0.02030, 0.02020, 0.01990, 0.01941 };
//const static float weight[4] = { 0.00159151943, 0.001518361155, 0.00156003464, 0.49612584449 };
#define TOTAL_WEIGHT 5

float4 main(PS_INPUT input) : SV_TARGET0
{
	float3 centerColor = scene.Sample(defaultSampler, input.textCoord).xyz;
	float3 pixelColor = centerColor * weight[0];

	float2 texelSize = float2(1.f / width, 1.f / height);

	float normalization = 1.0f;

	float factor = length(float3(1, 1, 1));

	if (type == 0)
	{
		//horizontal
		[unroll] for (int i = 1; i < TOTAL_WEIGHT; i++)
		{
			float3 rColor = scene.Sample(defaultSampler, input.textCoord + float2(texelSize.x * i, 0.0f)).xyz;
			float3 lColor = scene.Sample(defaultSampler, input.textCoord - float2(texelSize.x * i, 0.0f)).xyz;


			float rCloseness = 1.0f - distance(rColor, centerColor) / factor;
			float lCloseness = 1.0f - distance(lColor, centerColor) / factor;


			float rWeight = rCloseness * weight[i];
			float lWeight = lCloseness * weight[i];

			pixelColor += rColor * rWeight;
			pixelColor += lColor * lWeight;

			//normalization += rWeight + lWeight;
		}

		pixelColor /= normalization;
	}
	else
	{
		//vertical
		[unroll] for (int i = 1; i < TOTAL_WEIGHT; i++)
		{
			float3 rColor = scene.Sample(defaultSampler, input.textCoord + float2(0.0f, texelSize.y * i)).xyz;
			float3 lColor = scene.Sample(defaultSampler, input.textCoord - float2(0.0f, texelSize.y * i)).xyz;


			float rCloseness = 1.0f - distance(rColor, centerColor) / factor;
			float lCloseness = 1.0f - distance(lColor, centerColor) / factor;


			float rWeight = rCloseness * weight[i];
			float lWeight = lCloseness * weight[i];

			pixelColor += rColor * rWeight;
			pixelColor += lColor * lWeight;

			//normalization += rWeight + lWeight;
		}

		pixelColor /= normalization;
	}

	/*int x = floor(input.textCoord * screenWidth);
	int y = floor(input.textCoord * screenHeight);

	float ditherValue = ditherPattern[x % 4][y % 4];*/

	return float4(pixelColor, 1.0f);
}