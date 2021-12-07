struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 textCoord : POSITION;
};

SamplerState defaultSampler : register(s0);
TextureCube cubeMap : register(t5);

float4 main(PS_INPUT input) : SV_TARGET0
{
	//float3 texcoord = input.textCoord;
	//texcoord.z = 0.5f;
	return cubeMap.Sample(defaultSampler, input.textCoord);
}