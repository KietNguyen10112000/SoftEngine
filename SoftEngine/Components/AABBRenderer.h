#pragma once

#include <vector>

#include "Math/AABB.h"

class VertexBuffer;
class RenderPipeline;
class IRenderer;

class AABBRenderer
{
public:
	inline static constexpr size_t MAX_AABB_PER_FRAME = 10000;

public:
	VertexBuffer* m_vb = 0;

	RenderPipeline* m_rpl = 0;

	std::vector<AABB> m_aabbs;

public:
	AABBRenderer();
	~AABBRenderer();

public:
	void Present(IRenderer* renderer);

public:
	void Add(const AABB& aabb);

};