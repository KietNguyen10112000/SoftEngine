struct VS_INPUT
{
	float3 position				: POSITION;
	float2 textCoord			: TEXTCOORD;
	float3 tangent				: TANGENT;
	float3 bitangent			: BITANGENT;
	float3 normal				: NORMAL;
	float4 row1					: INSTANCE_TRANSFORM_ROW1;
	float4 row2					: INSTANCE_TRANSFORM_ROW2;
	float4 row3					: INSTANCE_TRANSFORM_ROW3;
	float4 row4					: INSTANCE_TRANSFORM_ROW4;
	uint instanceID				: SV_InstanceID;
};


struct VS_OUTPUT
{
	float4 pos			: SV_POSITION;
	float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
	float3x3 TBN		: TBN_MATRIX;
};

cbuffer Transform : register(b0)
{
	//per instance
	row_major float4x4 transform;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 mvp;
};

cbuffer LocalTransform : register(b2)
{
	row_major float4x4 localTransform;
};

cbuffer InstancesTransform : register(b3)
{
	//all instances
	row_major float4x4 instancesTransform;
};

static float4x4 toWorldSpace = mul(localTransform, transform);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	/*float4 pos = mul(float4(input.position, 1.0f), localTransform);
	pos += float4(input.instancePos, 0);
	output.pos = mul(pos, transform);*/

	//output.pos = mul(float4(input.position, 1.0f), toWorldSpace);
	//output.pos += float4(input.row4.xyz, 0);

	float4x4 insTransform = float4x4(input.row1, input.row2, input.row3, input.row4);
	output.pos = mul(mul(float4(input.position, 1.0f), toWorldSpace), insTransform);
	output.pos = mul(output.pos, instancesTransform);
	//output.pos = mul(output.pos, insTransform);


	output.position = output.pos;

	output.pos = mul(output.pos, mvp);

	output.textCoord = input.textCoord;


	float3 t = normalize(mul(float4(input.tangent, 0.0), toWorldSpace).xyz);
	float3 b = normalize(mul(float4(input.bitangent, 0.0), toWorldSpace).xyz);
	float3 n = normalize(mul(float4(input.normal, 0.0), toWorldSpace).xyz);

	output.TBN = float3x3(t, b, n);

	return output;
}