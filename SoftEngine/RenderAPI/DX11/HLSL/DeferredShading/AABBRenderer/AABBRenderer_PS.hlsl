struct PS_INPUT
{
	float4 pos			: SV_POSITION;
};

struct PS_OUTPUT
{
	float4 color		: SV_TARGET0;
	//float4 normAndSpec	: SV_TARGET2;
};

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	output.color = float4(0, 1, 0, 1);
	//output.normAndSpec = float4(0, 0, 0, 0);
	return output;
}