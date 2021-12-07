struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler : register(s0);

Texture2D depthMap						: register(t3);
Texture2D lastScene						: register(t4);
Texture2D normAndShininess				: register(t6);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 pixelColor = lastScene.Sample(defaultSampler, input.textCoord);
	return pixelColor;
}