#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "GraphicsFundamental.h"

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

	CubeRenderer m_wireFrameCubeRenderer;
	CubeRenderer m_solidCubeRenderer;

	PyramidRenderer m_pyramidRenderer;

public:
	DebugGraphics();

private:
	void InitCubeRenderer();

public:
	//void SetWireframe(bool on);

	void DrawDirection(const Vec3& origin, const Vec3& direction, const Vec4& headColor = { 0,1,0,1 }, const Vec4& tailColor = { 1,1,0,1 });
	void DrawAABox(const AABox& aaBox, const Vec4& color = { 1,1,1,1 });
	void DrawCube(const Mat4& transform, const Vec4& color = { 1,1,1,1 });
	void DrawFrustum(const Frustum& frustum, const Vec4& color = { 1,1,1,1 });

public:
	void RenderToTarget(GraphicsRenderTarget* renderTarget, GraphicsDepthStencilBuffer* depthBuffer, 
		SharedPtr<GraphicsConstantBuffer>& cameraBuffer);

};

NAMESPACE_END