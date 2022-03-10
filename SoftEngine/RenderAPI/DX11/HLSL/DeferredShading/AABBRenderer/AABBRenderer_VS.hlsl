struct IO
{
	float3 position		: POSITION;
	float3 dimensions	: DIMENSIONS;
};

IO main(IO input)
{
	return input;
}