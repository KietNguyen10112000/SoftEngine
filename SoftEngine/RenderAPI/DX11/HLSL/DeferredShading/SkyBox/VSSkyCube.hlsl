struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 textCoord : POSITION;
};

cbuffer Transform : register(b0)
{
	row_major float4x4 transform;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 mvp;
};

VS_OUTPUT main(float3 pos : POSITION)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(float4(pos, 1.0f), transform);

	//output.pos = float4(pos, 1.0f);

	output.pos = mul(output.pos, mvp);

	output.pos.z = output.pos.w - 0.000001f;

	//output.textCoord.x = output.pos.x / output.pos.w;
	//output.textCoord.y = output.pos.y / output.pos.w;
	//output.textCoord.z = 1;

	output.textCoord = pos;

	return output;
}