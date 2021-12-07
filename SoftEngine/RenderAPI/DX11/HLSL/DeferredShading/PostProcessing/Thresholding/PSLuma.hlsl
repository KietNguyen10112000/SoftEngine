struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler : register(s0);

Texture2D depthMap						: register(t3);
Texture2D scene							: register(t4);

cbuffer Info : register(b2)
{
	float thres;
	float3 factor;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).xyz;

	float brightness = dot(pixelColor, factor);
	if (brightness > thres)
		return float4(pixelColor.rgb, 1.0);
	else
		return float4(0, 0, 0, 1);
}