#define PI 3.14159265359f

struct VS_INPUT
{
	float3 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3 normal			: NORMAL;
};


struct VS_OUTPUT
{
	float4 pos				: SV_POSITION;
	float4 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3x3 TBN			: TBN_MATRIX;

	float depth				: DEPTH;

	float  alpha			: ALPHA;
};

cbuffer Transform : register(b0)
{
	row_major float4x4 transform;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 mvp;
};

cbuffer Info : register(b4)
{
	float highUnit;
	float xUnit;
	float zUnit;
	float t;
};

struct Vertex
{
	float3 translation;
	float3 T;
	float3 B;
};

struct GerstnerWave
{
	float steepness;
	float waveLength;
	float speed;
	float2 dir;
};

Vertex DoGerstnerWave(float3 pos, GerstnerWave wave)
{
	Vertex ret;

	float3 tempPos = float3(0, 0, 0);
	float steepness = wave.steepness;
	float k = 2 * PI / wave.waveLength;
	float speed = wave.speed;
	float A = steepness / k;
	float2 d = normalize(wave.dir);

	float f = k * (dot(d, pos.xz) - speed * t);
	float Acosf = A * cos(f);
	float Asinf = A * sin(f);

	tempPos.x = d.x * Acosf;
	tempPos.y = Asinf;
	tempPos.z = d.y * Acosf;

	float3 tangent = float3(
		1 - d.x * d.x * (Asinf)*k,
		d.x * (Acosf)*k,
		-d.x * d.y * (Asinf)*k
		);
	float3 binormal = float3(
		-d.x * d.y * (Asinf)*k,
		d.y * (Acosf)*k,
		1 - d.y * d.y * (Asinf)*k
		);

	ret.translation = tempPos;
	ret.T = tangent;
	ret.B = binormal;

	return ret;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	float3 tempPos = input.position;
	float3 T = float3(1, 0, 0);
	float3 B = float3(0, 0, 1);

	//float time = t;
	//float amplitude = 1.5f;
	//float lamda = 20;
	//float coef = 0.05f;
	//float omega = 2;

	//float dist = distance(float2(0, 20), float2(tempPos.x, tempPos.z));
	////wavy water source 1
	//tempPos.y = amplitude * cos(omega * time - 2 * PI * dist / lamda)
	//	+ amplitude * (dist / lamda) * coef * cos(omega * time - 2 * PI * dist / lamda + PI);


	////wavy water source 2
	//dist = distance(float2(20, 0), float2(tempPos.x, tempPos.z));
	//tempPos.y += (amplitude * 0.8) * cos((omega / 2) * time - 2 * PI * dist / lamda + PI / 3)
	//	+ (amplitude * 0.8) * (dist / lamda) * coef * cos((omega / 2) * time - 2 * PI * dist / lamda + PI + PI / 3);


	GerstnerWave wave1 = { 0.4f, 10, 4, float2(1,1) };
	GerstnerWave wave2 = { 0.3f, 20, 2, float2(0,1) };
	GerstnerWave wave3 = { 0.2f, 30, 3, float2(1,0) };

	Vertex t = DoGerstnerWave(input.position, wave1);

	tempPos += t.translation;
	T += t.T;
	B += t.B;

	t = DoGerstnerWave(input.position, wave2);

	tempPos += t.translation;
	T += t.T;
	B += t.B;

	t = DoGerstnerWave(input.position, wave3);

	tempPos += t.translation;
	T += t.T;
	B += t.B;

	//float3 normal = normalize(cross(B, T));

	/*output.maxA = 
		wave1.steepness / (2 * PI / wave1.waveLength)
		+ wave2.steepness / (2 * PI / wave2.waveLength)
		+ wave3.steepness / (2 * PI / wave3.waveLength);

	output.oldPos = input.position.xyz;*/

	output.pos = mul(float4(tempPos, 1.0f), transform);

	output.position = output.pos;

	output.pos = mul(output.pos, mvp);

	output.textCoord = input.textCoord;

	float3 _t = normalize(mul(float4(T, 0.0), transform).xyz);
	float3 _b = normalize(mul(float4(B, 0.0), transform).xyz);
	float3 _n = normalize(mul(float4(cross(B, T), 0.0), transform).xyz);

	output.TBN = float3x3(_t, _b, _n);

	output.alpha = 0.84f;

	output.depth = output.pos.z / output.pos.w;

	return output;
}