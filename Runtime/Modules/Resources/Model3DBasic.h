#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

// basic model3d with vertex and uv (textcoord), all static meshes
class Model3DBasic : public ResourceBase
{
public:
	struct Vertex
	{
		Vec3 position;
		Vec3 tangent;
		Vec3 bitangent;
		Vec3 normal;
		Vec2 textcoord;
	};

	class Mesh
	{
	public:
		SharedPtr<GraphicsVertexBuffer> m_vertexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_model3DIdx;
		AABox m_aabb;

		inline const auto& GetVertexBuffer() const
		{
			return m_vertexBuffer;
		}

		inline auto GetVertexCount() const
		{
			return m_vertexCount;
		}

		inline AABox GetLocalAABB() const
		{
			return m_aabb;
		}
	};

	std::vector<Mesh> m_meshes;

	Model3DBasic(String path, bool placeholder = false);

};

NAMESPACE_END