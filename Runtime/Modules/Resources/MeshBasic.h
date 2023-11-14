#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

class Model3DBasic;

// basic model3d with vertex and uv (textcoord)
class MeshBasic : public ResourceBase
{
public:
	struct Vertex
	{
		Vec3 position;
		Vec2 textcoord;
	};

	// must be raw ptr to avoid cyclic (can use Handle<> but this is just resource :D)
	Model3DBasic* m_model3D;
	SharedPtr<GraphicsVertexBuffer> m_vertexBuffer;
	uint32_t m_vertexCount;
	uint32_t m_model3DIdx;
	AABox m_aabb;

	MeshBasic(String path);

public:
	inline const auto& GetVertexBuffer() const
	{
		return m_vertexBuffer;
	}

	inline auto GetVertexCount() const
	{
		return m_vertexCount;
	}

	AABox GetLocalAABB() const;

};

NAMESPACE_END