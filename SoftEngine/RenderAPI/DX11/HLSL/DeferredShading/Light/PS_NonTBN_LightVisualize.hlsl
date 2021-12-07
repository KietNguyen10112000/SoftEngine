struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
	float3 normal		: NORMAL;
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

Texture2D diffuseMap : register(t0);

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = diffuseMap.Sample(defaultSampler, input.textCoord);

	output.color.xyz = output.color.xyz * diffuse;

	output.color.xyz += ambient.xyz * output.color.xyz;

	//float3 norm = input.normal;

	output.normAndShininess = float4(float3(0,0,0), shininess);

	output.positionAndSpec = float4(input.position.xyz, specular);

	return output;
}