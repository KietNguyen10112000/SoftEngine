#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

// basic model3d with vertex and uv (textcoord)
class Model3DBasic : public ResourceBase
{
public:
	struct Vertex
	{
		Vec3 position;
		Vec2 textcoord;
	};

	SharedPtr<GraphicsVertexBuffer> m_vertexBuffer;
	uint32_t m_vertexCount;

	Model3DBasic(String path);

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