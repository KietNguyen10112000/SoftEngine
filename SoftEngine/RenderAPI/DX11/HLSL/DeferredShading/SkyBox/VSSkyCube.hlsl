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
	row_major float4x4 view;
	row_major float4x4 proj;
};

VS_OUTPUT main(float3 pos : POSITION)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(float4(pos, 1.0f), transform);

	//output.pos = float4(pos, 1.0f);

	/*float4x4 temp = view;
	temp._m30 = 0;
	temp._m31 = 0;
	temp._m32 = 0;*/

	output.pos = mul(output.pos, mvp);
	//output.pos = mul(output.pos, proj);

	output.pos.z = output.pos.w - 0.000001f;

	//output.textCoord.x = output.pos.x / output.pos.w;
	//output.textCoord.y = output.pos.y / output.pos.w;
	//output.textCoord.z = 1;

	output.textCoord = pos;

	return output;
}