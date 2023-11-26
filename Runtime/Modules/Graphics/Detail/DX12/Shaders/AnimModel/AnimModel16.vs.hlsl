#define MAX_BONE 512

#include "../Common/TypeDef.hlsli"

struct VS_INPUT
{
	float3 pos							: POSITION;
	float3 tangent						: TANGENT;
	float3 bitangent					: BITANGENT;
	float3 normal						: NORMAL;
	float2 textCoord					: TEXTCOORD;

	vector <min16uint, 4> boneId1		: BONE1;
	vector <min16uint, 4> boneId2		: BONE2;
	vector <min16uint, 4> boneId3		: BONE3;
	vector <min16uint, 4> boneId4		: BONE4;

	float4 weight1						: WEIGHT1;
	float4 weight2						: WEIGHT2;
	float4 weight3						: WEIGHT3;
	float4 weight4						: WEIGHT4;
};

struct VS_OUTPUT
{
	float4 svposition	: SV_POSITION;
	float3 position		: POSITION;
	float3x3 TBN		: TBN_MATRIX;
	float2 textCoord	: TEXTCOORD;
};

cbuffer CameraCBuffer : register(b0, SPACE_VS)
{
	CameraData Camera;
};

//cbuffer ObjectCBuffer : register(b1, SPACE_VS)
//{
//	ObjectData Object;
//};

cbuffer Bones : register(b1, SPACE_VS)
{
	row_major float4x4 bones[MAX_BONE];
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	float4 position = float4(input.pos, 1.0f);

	float4x4 boneTransform = bones[input.boneId1.x] * input.weight1.x;
	boneTransform += bones[input.boneId1.y] * input.weight1.y;
	boneTransform += bones[input.boneId1.z] * input.weight1.z;
	boneTransform += bones[input.boneId1.w] * input.weight1.w;

	boneTransform += bones[input.boneId2.x] * input.weight2.x;
	boneTransform += bones[input.boneId2.y] * input.weight2.y;
	boneTransform += bones[input.boneId2.z] * input.weight2.z;
	boneTransform += bones[input.boneId2.w] * input.weight2.w;

	boneTransform += bones[input.boneId3.x] * input.weight3.x;
	boneTransform += bones[input.boneId3.y] * input.weight3.y;
	boneTransform += bones[input.boneId3.z] * input.weight3.z;
	boneTransform += bones[input.boneId3.w] * input.weight3.w;

	boneTransform += bones[input.boneId4.x] * input.weight4.x;
	boneTransform += bones[input.boneId4.y] * input.weight4.y;
	boneTransform += bones[input.boneId4.z] * input.weight4.z;
	boneTransform += bones[input.boneId4.w] * input.weight4.w;

	//float4x4 temp = mul(boneTransform, Object.transform);
	float4x4 temp = boneTransform;
	position = mul(position, temp);

	//output.position = output.pos;
	position /= position.w;

	output.position = position.xyz;

	output.svposition = mul(position, Camera.vp);
	output.textCoord = input.textCoord;


	float3 t = normalize(mul(float4(input.tangent, 0.0), temp).xyz);
	float3 b = normalize(mul(float4(input.bitangent, 0.0), temp).xyz);
	float3 n = normalize(mul(float4(input.normal, 0.0), temp).xyz);

	output.TBN = float3x3(t, b, n);

	return output;
}