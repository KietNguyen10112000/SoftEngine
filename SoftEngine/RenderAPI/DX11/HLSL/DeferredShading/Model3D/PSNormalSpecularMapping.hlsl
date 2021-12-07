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
	float4 ambient;
	float3 diffuse;
	float shininess;
};

SamplerState defaultSampler : register(s0);

Texture2D diffuseMap		: register(t0);
Texture2D normalMap			: register(t1);
Texture2D specularMap		: register(t2);

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = diffuseMap.Sample(defaultSampler, input.textCoord);

	output.color.xyz = output.color.xyz * diffuse;

	output.color.xyz += ambient.xyz * output.color.xyz;

	float3 norm = normalMap.Sample(defaultSampler, input.textCoord).xyz;

	norm = norm * 2 - 1;

	norm = mul(norm, input.TBN);

	float spec = specularMap.Sample(defaultSampler, input.textCoord).x;

	output.normAndShininess = float4(norm, shininess);

	output.positionAndSpec = float4(input.position.xyz, spec);

	return output;
}