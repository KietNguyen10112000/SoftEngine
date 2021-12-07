struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
	float3x3 TBN		: TBN_MATRIX;
};

struct PS_OUTPUT
{
	float4 color				: SV_TARGET0;
	float4 positionAndSpec		: SV_TARGET1;
	float4 normAndShininess		: SV_TARGET2;
};

cbuffer Material : register(b2)
{
	float3 ambient;
	float specular;
	float3 diffuse;
	float shininess;
};

SamplerState defaultSampler : register(s0);

Texture2D diffuseMap		: register(t0);
Texture2D normalMap			: register(t1);

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = diffuseMap.Sample(defaultSampler, input.textCoord);

	output.color.xyz = output.color.xyz * diffuse;

	output.color.xyz += ambient.xyz * output.color.xyz;

	float3 norm = normalMap.Sample(defaultSampler, input.textCoord).rgb;

	//if (abs(norm.r - 0.5f) < 0.1f) norm = float3(1, 0, 0);
	//norm = float3(norm.r, 0, 0);

	norm = norm * 2 - 1;

	//float3 norm = float3(0.003f, 0.003f, 1);

	norm = mul(norm, input.TBN);
	
	output.normAndShininess = float4(norm, shininess);

	output.positionAndSpec = float4(input.position.xyz, specular);

	return output;
}