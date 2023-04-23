#include "../TypeDef.hlsli"

#include "../Primitives/CubeVertices.hlsli"

struct VS_INPUT
{
    uint   vertexId             : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position		: SV_POSITION;
    float4 color        : COLOR;
};

cbuffer SceneCBuffer : register(b0)
{
    SceneData Scene;
};

cbuffer CameraCBuffer : register(b1)
{
    CameraData Camera;
};

cbuffer ObjectCBuffer : register(b2)
{
    ObjectData Object;
    float4     Color;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

    float3 pos = CUBE_VERTICES[CUBE_INDICES[input.vertexId]];
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, Object.transform);

    output.position /= output.position.w;

	output.position = mul(output.position, Camera.vp);

    output.color = Color;

	return output;
}