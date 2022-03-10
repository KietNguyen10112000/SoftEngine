struct GS_INPUT
{
	float3 position		: POSITION;
	float3 dimensions	: DIMENSIONS;
};

struct GS_OUTPUT
{
	float4 pos			: SV_POSITION;
};

#define PADDING 0.2f

cbuffer Camera : register(b1)
{
    row_major float4x4 mvp;
};

[maxvertexcount(24)]
void main(inout LineStream<GS_OUTPUT> output, point GS_INPUT input[1])
{
    float3 pos = input[0].position;
    float3 dim = input[0].dimensions;

    pos -= float3(PADDING, PADDING, PADDING);
    dim += float3(PADDING, PADDING, PADDING) * 2.0f;

    const float3 n1 = pos;
    const float3 n2 = pos + float3(dim.x, 0, 0);
    const float3 n3 = pos + float3(0, 0, dim.z);
    const float3 n4 = pos + float3(dim.x, 0, dim.z);

    const float3 n5 = pos + float3(0, dim.y, 0);
    const float3 n6 = pos + float3(dim.x, dim.y, 0);
    const float3 n7 = pos + float3(0, dim.y, dim.z);
    const float3 n8 = pos + float3(dim.x, dim.y, dim.z);

    /*const float3 cubeVertices[] = {
        n1, n2, n3,
        n3, n4, n2,

        n5, n6, n7,
        n7, n8, n6,

        n1, n2, n5,
        n5, n6, n2,

        n3, n4, n7,
        n7, n8, n4,

        n1, n3, n5,
        n5, n7, n3,

        n2, n4, n6,
        n6, n8, n4
    };*/

    const float3 cubeVertices[] = {
        n1, n2,
        n1, n3,
        n4, n3,
        n4, n2,

        n5, n7,
        n5, n6,
        n8, n6, 
        n8, n7,

        n2, n6,
        n1, n5,

        n7, n3,
        n8, n4
    };

    GS_OUTPUT outp;

    /*[unroll] for (uint i = 0; i < 12; i++)
    {
        uint id = i * 3;

        [unroll] for (uint j = 0; j < 3; j++)
        {
            uint jj = (j + 1) % 3;

            outp.pos = float4(cubeVertices[id + j], 1.0f);
            outp.pos = mul(outp.pos, mvp);
            output.Append(outp);

            outp.pos = float4(cubeVertices[id + jj], 1.0f);
            outp.pos = mul(outp.pos, mvp);
            output.Append(outp);

            output.RestartStrip();
        }
    }*/

    [unroll] for (uint i = 0; i < 12; i++)
    {
        uint id = i * 2;

        outp.pos = float4(cubeVertices[id], 1.0f);
        outp.pos = mul(outp.pos, mvp);
        output.Append(outp);

        outp.pos = float4(cubeVertices[id + 1], 1.0f);
        outp.pos = mul(outp.pos, mvp);
        output.Append(outp);

        output.RestartStrip();
    }
}