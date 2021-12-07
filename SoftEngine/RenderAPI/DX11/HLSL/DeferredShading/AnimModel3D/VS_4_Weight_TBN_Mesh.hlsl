#define MAX_BONE 512

struct VS_INPUT
{
	float3 pos							: POSITION;
	float2 textCoord					: TEXTCOORD;
	float3 tangent						: TANGENT;
	float3 bitangent					: BITANGENT;

	vector <min16uint, 4> boneId1		: BONE1;
	float4 weight1						: WEIGHT1;
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

	output.pos = float4(input.pos, 1.0f);

	float4x4 boneTransform = bones[input.boneId1.x] * input.weight1.x;
	boneTransform += bones[input.boneId1.y] * input.weight1.y;
	boneTransform += bones[input.boneId1.z] * input.weight1.z;
	boneTransform += bones[input.boneId1.w] * input.weight1.w;


	float4x4 temp = mul(boneTransform, world);
	output.pos = mul(output.pos, temp);

	output.position = output.pos;

	output.pos = mul(output.pos, mvp);
	output.textCoord = input.textCoord;


	float3 t = normalize(mul(float4(input.tangent, 0.0), temp).xyz);
	float3 b = normalize(mul(float4(input.bitangent, 0.0), temp).xyz);
	float3 n = normalize(cross(b, t));

	output.TBN = float3x3(t, b, n);


	return output;
}