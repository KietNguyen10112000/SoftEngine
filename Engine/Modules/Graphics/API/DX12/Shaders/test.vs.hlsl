struct VS_INPUT
{
	float3 position		: POSITION;
	float3 color		: COLOR;
};

struct VS_OUTPUT
{
	float4 position		: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	//VS_OUTPUT output;

	//output.position = mul(float4(input.position, 1.0f), Object.transform);
	//output.position = mul(output.position, Camera.vp);
	//output.color = input.color;
    
    
    output.position = float4(input.position, 1.0f);
	output.textCoord = input.color.xy;

	return output;
}