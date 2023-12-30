#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "GraphicsFundamental.h"

#include "Resources/Model3DBasic.h"

NAMESPACE_BEGIN

class API DebugGraphics
{
private:
	struct CubeRenderer
	{
		constexpr static size_t BATCH_SIZE = 512;

		SharedPtr<GraphicsPipeline> pipeline;
		SharedPtr<GraphicsConstantBuffer> constantBuffer;

		struct CBuffer
		{
			Mat4 transforms[BATCH_SIZE];
			Vec4 colors[BATCH_SIZE];
		};

		size_t backCount = 0;
		std::vector<CBuffer> buffers;

		spinlock lock;
		bool padd[3];

		void Init(bool wireframe);
		void Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer);
	};

	struct PyramidRenderer
	{
		constexpr static size_t BATCH_SIZE = 512;

		SharedPtr<GraphicsPipeline> pipeline;
		SharedPtr<GraphicsConstantBuffer> constantBuffer;

		struct CBuffer
		{
			Mat4 transforms[BATCH_SIZE];
			Vec4 colors[BATCH_SIZE];
		};

		size_t backCount = 0;
		std::vector<CBuffer> buffers;

		spinlock lock;
		bool padd[3];

		void Init();
		void Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer);
	};

	struct Model3DRenderer
	{
		constexpr static size_t BATCH_SIZE = 512;

		SharedPtr<GraphicsPipeline> pipeline;
		SharedPtr<GraphicsConstantBuffer> constantBuffer;

		Resource<Model3DBasic> model;

		struct CBuffer
		{
			Mat4 transforms[BATCH_SIZE];
			Vec4 colors[BATCH_SIZE];
		};

		size_t backCount = 0;
		std::vector<CBuffer> buffers;

		spinlock lock;
		bool padd[3];

		void Init(bool wireframe, const SharedPtr<GraphicsPipeline>& sharedPipeline, const SharedPtr<GraphicsConstantBuffer>& sharedConstantBuffer, const char* customVS = nullptr);
		void Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer);
	};

	struct DebugMeshRenderer
	{
		constexpr static size_t BATCH_SIZE = 512;

		constexpr static size_t NUM_VERTEX_BUFFERS = 16;
		constexpr static size_t VERTEX_BUFFER_SIZE = 3 * MB;
		constexpr static size_t VERTEX_BUFFER_ELEMENTS_COUNT = VERTEX_BUFFER_SIZE / sizeof(Vec3);

		SharedPtr<GraphicsPipeline> pipeline;
		SharedPtr<GraphicsConstantBuffer> constantBuffer;

		SharedPtr<GraphicsVertexBuffer> vertexBuffers[NUM_VERTEX_BUFFERS];

		struct Data
		{
			Mat4 transform;
			Vec4 color;

			// size refer to m_vertices
			size_t verticesCount;
		};

		struct CBuffer
		{
			Mat4 transform;
			Vec4 color;
		};

		std::vector<Data> dataPerDrawCall;

		std::vector<Vec3> vertices;

		spinlock lock;
		bool padd[3];

		void Init(bool wireframe);
		void Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer);
	};

	CubeRenderer m_wireFrameCubeRenderer;
	CubeRenderer m_solidCubeRenderer;

	PyramidRenderer m_pyramidRenderer;

	Model3DRenderer m_sphereRenderer;
	Model3DRenderer m_capsuleRenderer;

	DebugMeshRenderer m_meshRenderer;;

public:
	DebugGraphics();

private:
	void InitCubeRenderer();
	void InitSphereRenderer();

public:
	//void SetWireframe(bool on);

	void DrawDirection(const Vec3& origin, const Vec3& direction, const Vec4& headColor = { 0,1,0,1 }, const Vec4& tailColor = { 1,1,0,1 });
	void DrawAABox(const AABox& aaBox, const Vec4& color = { 1,1,1,1 });
	void DrawCube(const Mat4& transform, const Vec4& color = { 1,1,1,1 });
	void DrawFrustum(const Frustum& frustum, const Vec4& color = { 1,1,1,1 });

	void DrawSphere(const Sphere& sphere, const Vec4& color = { 1,1,1,1 });
	void DrawCapsule(const Capsule& capsule, const Vec4& color = { 1,1,1,1 });

	// hungry perfromance consumption
	// count <= 64 KB
	void DrawMesh(const Vec3* vertices, uint32_t count, const Mat4& transform = {}, const Vec4& color = { 1,1,1,1 });

public:
	void RenderToTarget(GraphicsRenderTarget* renderTarget, GraphicsDepthStencilBuffer* depthBuffer, 
		SharedPtr<GraphicsConstantBuffer>& cameraBuffer);

};

NAMESPACE_END