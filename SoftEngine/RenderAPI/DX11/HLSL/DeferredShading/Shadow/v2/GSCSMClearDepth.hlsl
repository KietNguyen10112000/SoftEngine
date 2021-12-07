#define NUM_CASCADE 4

struct GS_INPUT
{
    float4 pos			: SV_POSITION;
    float2 textCoord	: TEXTCOORD;
};

struct GS_OUTPUT
{
    float4 pos						: SV_POSITION;
    float2 textCoord	            : TEXTCOORD;
    uint   viewport                 : SV_ViewportArrayIndex;
};

cbuffer ShadowInfo : register(b0)
{
    uint index;
    float3 padding;
};

[maxvertexcount(18)]
void main(inout TriangleStream<GS_OUTPUT> output, triangle GS_INPUT input[3])
{
    [unroll] for (uint i = 0; i < NUM_CASCADE; i++)
    {
        GS_OUTPUT outp;
        [unroll] for (uint j = 0; j < 3; j++)
        {
            outp.pos = input[j].pos;
            outp.textCoord = input[j].textCoord;
            outp.viewport = i;
            output.Append(outp);
        }
        output.RestartStrip();
    }

}