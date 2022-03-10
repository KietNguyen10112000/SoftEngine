#define MAX_BONE 512

struct VS_INPUT
{
	float3 pos							: POSITION;
	float2 textCoord					: TEXTCOORD;
	float3 normal						: NORMAL;
	float3 tangent						: TANGENT;
	float3 bitangent					: BITANGENT;

	vector <min16uint, 4> boneId1		: BONE1;
	vector <min16uint, 4> boneId2		: BONE2;

	float4 weight1						: WEIGHT1;
	float4 weight2						: WEIGHT2;
};

struct VS_OUTPUT
{
	float4 pos						: SV_POSITION;
	float4 position					: POSITION;
	float2 textCoord				: TEXTCOORD;
	float3x3 TBN					: TBN_MATRIX;
};

cbuffer WorldBuffer : register(b0)
{
	row_major float4x4 world;
};

cbuffer CamBuffer : register(b1)
{
	row_major float4x4 mvp;
};

//cbuffer MeshWorldBuffer : register(b2)
//{
//	row_major float4x4 meshWorld;
//};

cbuffer Bones : register(b3)
{
	row_major float4x4 bones[MAX_BONE];
};

//static float4x4 toWorldSpace = mul(meshWorld, world);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.position = float4(input.pos, 1.0f);

	float4x4 boneTransform = bones[input.boneId1.x] * input.weight1.x;
	boneTransform += bones[input.boneId1.y] * input.weight1.y;
	boneTransform += bones[input.boneId1.z] * input.weight1.z;
	boneTransform += bones[input.boneId1.w] * input.weight1.w;

	boneTransform += bones[input.boneId2.x] * input.weight2.x;
	boneTransform += bones[input.boneId2.y] * input.weight2.y;
	boneTransform += bones[input.boneId2.z] * input.weight2.z;
	boneTransform += bones[input.boneId2.w] * input.weight2.w;

	float4x4 temp = mul(boneTransform, world);
	output.position = mul(output.position, temp);

	//output.position = output.pos;

	output.position /= output.position.w;

	output.pos = mul(output.position, mvp);
	output.textCoord = input.textCoord;


	float3 t = normalize(mul(float4(input.tangent, 0.0), temp).xyz);
	float3 b = normalize(mul(float4(input.bitangent, 0.0), temp).xyz);
	float3 n = normalize(mul(float4(input.normal, 0.0), temp).xyz);

	output.TBN = float3x3(t, b, n);

	return output;
}