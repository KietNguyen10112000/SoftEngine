struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler : register(s0);

Texture2D depthMap						: register(t3);
Texture2D scene							: register(t4);
Texture2D blured						: register(t5);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 pixelColor = scene.Sample(defaultSampler, input.textCoord);
	float4 bluredColor = blured.Sample(defaultSampler, input.textCoord);
	return float4((pixelColor.xyz + bluredColor.xyz), 1.0f);
}