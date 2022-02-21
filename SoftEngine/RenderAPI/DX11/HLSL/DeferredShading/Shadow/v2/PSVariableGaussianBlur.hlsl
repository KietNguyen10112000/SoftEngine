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
	uint halfTotalWeight;

	float width;
	float height;

	float4 weights[4];
};


float4 main(PS_INPUT input) : SV_TARGET0
{
	float4 pixelColor = scene.Sample(defaultSampler, input.textCoord) * weights[0].x;

	float2 texelSize = float2(1.f / width, 1.f / height);

	if (type == 0)
	{
		//horizontal
		[unroll] for (int i = 1; i < halfTotalWeight; i++)
		{
			float weight = ((float[4])(weights[i / 4]))[i % 4];
			pixelColor += scene.Sample(defaultSampler, input.textCoord + float2(texelSize.x * i, 0.0f)) * weight;
			pixelColor += scene.Sample(defaultSampler, input.textCoord - float2(texelSize.x * i, 0.0f)) * weight;
		}
	}
	else
	{
		//vertical
		[unroll] for (int i = 1; i < halfTotalWeight; i++)
		{
			float weight = ((float[4])(weights[i / 4]))[i % 4];
			pixelColor += scene.Sample(defaultSampler, input.textCoord + float2(0.0f, texelSize.y * i)) * weight;
			pixelColor += scene.Sample(defaultSampler, input.textCoord - float2(0.0f, texelSize.y * i)) * weight;
		}
	}

	return pixelColor;
}