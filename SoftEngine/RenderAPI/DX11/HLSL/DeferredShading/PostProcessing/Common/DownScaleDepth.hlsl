struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler			: register(s0);

//always t3
Texture2D depthMap					: register(t3);

float4 main(PS_INPUT input) : SV_TARGET0
{
	float3 pixelColor = depthMap.Sample(defaultSampler, input.textCoord).rrr;

	return float4(pixelColor, 1.0f);
}