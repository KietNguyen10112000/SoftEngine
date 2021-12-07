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


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).xyz;

	float brightness = dot(pixelColor, float3(0.2126, 0.7152, 0.0722));
	if (brightness > 0.8f)
		return float4(pixelColor.rgb, 1.0);

	float lightSourceDepth = lightsPosOnScreen[0].z;
	
	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord).r;

	if (pixelDepth < lightSourceDepth) return float4(0, 0, 0, 1);

	return float4(0.196f, 0.2f, 0.278f, 1);
}