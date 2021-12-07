struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler				: register(s0);

Texture2D depthMap						: register(t3);
Texture2D scene							: register(t4);


#define NUM_SAMPLES 20
#define MAX_LIGHT 5

cbuffer Info : register(b2)
{
	uint  totalLights;
	float3 padding;
	float4 lightsPosOnScreen[MAX_LIGHT];
};

#define RESOLUTION_FACTOR 1.f

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord * RESOLUTION_FACTOR).xyz;

	//float2 NDC = lightsPosOnScreen[0].xy;
	//float2 NDCToUV = float2((NDC.x + 1.0f) / 2.0f, (1.0f + NDC.y) / 2.0f);
	float2 NDCToUV = lightsPosOnScreen[0].xy;

	/*if(NDCToUV.x > 1 || NDCToUV.x < 0 || NDCToUV.y > 1 || NDCToUV.y < 0)
	{
		return float4(pixelColor, 1.0f);
	}*/

	float2 deltaTextCoord = float2(input.textCoord - NDCToUV);

	deltaTextCoord = deltaTextCoord * (1.0 / (float(NUM_SAMPLES) * 4.5f));

	float illuminationDecay = 1.0;

	float2 textCoord = input.textCoord;

	float weight = 0.6;
	float decay = 0.8;

	[loop] for (int i = 0; i < NUM_SAMPLES; i++)
	{
		textCoord = textCoord - deltaTextCoord;

		float3 col = scene.Sample(defaultSampler, textCoord * RESOLUTION_FACTOR).xyz;

		col = col * illuminationDecay * weight;

		pixelColor += col;

		illuminationDecay *= decay;
	}

	float exposure = 0.5f;

	return float4(pixelColor * exposure, 1.0f);
}