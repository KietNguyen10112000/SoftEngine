#include "../Common/TypeDef.hlsli"

struct VS_INPUT
{
    float3 position		: POSITION;
	float3 tangent		: TANGENT;
	float3 bitangent	: BITANGENT;
	float3 normal		: NORMAL;
	float2 textCoord	: TEXTCOORD;
    
    uint vertexId       : SV_VertexID;
    uint instanceId     : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position     : SV_POSITION;
    float4 color        : COLOR;
};

cbuffer CameraCBuffer : register(b0, SPACE_VS)
{
    CameraData Camera;
};

cbuffer DataCBuffer : register(b1, SPACE_VS)
{
    row_major float4x4 transforms[512];
    float4 colors[512];
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float3 pos = input.position;
    float4x4 transform = transforms[input.instanceId];
    
    float scaleH = transform._m03;
    transform._m03 = 0.0f;
    
    float scaleR = transform._m13;
    transform._m13 = 0.0f;
    
    if (abs(pos.y) < 0.55f)
    {
        float4x4 scale =
        {
            scaleR, 0.f, 0.f, 0.f,
            0.f, scaleH, 0.f, 0.f,
            0.f, 0.f, scaleR, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
        transform = mul(scale, transform);
    } 
    else
    {
        float t = scaleH / 2.0f;
        float signY = sign(pos.y);
        
        float4x4 scale =
        {
            scaleR, 0.f, 0.f, 0.f,
            0.f, scaleR, 0.f, 0.f,
            0.f, 0.f, scaleR, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
        
        float4x4 translation =
        {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, t * signY, 0.f, 1.f
        };
        transform = mul(translation, transform);
        transform = mul(scale, transform);
        
        pos.y -= signY * 0.5f;

    }
    
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, transform);
    output.position = mul(output.position, Camera.vp);

    output.color = colors[input.instanceId];

    return output;
}