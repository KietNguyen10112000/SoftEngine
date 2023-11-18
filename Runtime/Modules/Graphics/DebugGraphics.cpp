#include "DebugGraphics.h"

#include "Graphics.h"

NAMESPACE_BEGIN

void DebugGraphics::CubeRenderer::Init(bool wireframe)
{
	auto graphics = Graphics::Get();

	auto& renderer = *this;
	renderer.backCount = 0;
	renderer.buffers.resize(1);

	GRAPHICS_PIPELINE_DESC desc = {};
	desc.preferRenderCallPerFrame = 512;
	desc.vs = "DebugGraphics/Cube.vs";
	desc.ps = "DebugGraphics/Px.ps";
	desc.inputDesc.numElements = 0;
	desc.outputDesc.numRenderTarget = 1;
	desc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	desc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;
	desc.rasterizerDesc.cullMode = GRAPHICS_CULL_MODE::BACK;
	desc.rasterizerDesc.fillMode = wireframe ? GRAPHICS_FILL_MODE::WIREFRAME : GRAPHICS_FILL_MODE::SOLID;

	renderer.pipeline = graphics->CreateRasterizerPipeline(desc);

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(CubeRenderer::CBuffer);
	cbDesc.perferNumRoom = 128;
	graphics->CreateConstantBuffers(1, &cbDesc, &renderer.constantBuffer);
}

void DebugGraphics::CubeRenderer::Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer)
{
	auto graphics = Graphics::Get();

	graphics->SetGraphicsPipeline(pipeline.get());

	for (auto& buffer : buffers)
	{
		uint32_t instanceCount = CubeRenderer::BATCH_SIZE;
		if (&buffer == &buffers.back())
		{
			instanceCount = backCount;
		}

		if (instanceCount == 0)
		{
			continue;
		}

		constantBuffer->UpdateBuffer(&buffer.transforms[0], sizeof(CubeRenderer::CBuffer));

		auto params = pipeline->PrepareRenderParams();
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &cameraBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &constantBuffer);

		graphics->DrawInstanced(0, 0, 36, instanceCount, 0, 0);
	}

	buffers.resize(1);
	backCount = 0;
}

void DebugGraphics::PyramidRenderer::Init()
{
	/*Vec3 vertices[100];
	Vec3 bvertices[100];
	size_t idx = 0;
	size_t bidx = 0;

	constexpr size_t NUM_SAMPLES = 10;

	float arc = 0;
	constexpr float dA = 2 * PI / (float)NUM_SAMPLES;
	for (size_t i = 0; i < 10; i++)
	{
		float x1 = std::cos(arc) / 15.0f;
		float z1 = std::sin(arc) / 15.0f;

		arc += dA;

		float x2 = std::cos(arc) / 15.0f;
		float z2 = std::sin(arc) / 15.0f;

		{
			auto& v1 = vertices[idx++];
			auto& v2 = vertices[idx++];
			auto& v3 = vertices[idx++];

			v1.x = 0;
			v1.y = 0.2f;
			v1.z = 0;

			v2.x = x1;
			v2.y = 0;
			v2.z = z1;

			v3.x = x2;
			v3.y = 0;
			v3.z = z2;
		}

		{
			auto& v1 = bvertices[bidx++];
			auto& v2 = bvertices[bidx++];
			auto& v3 = bvertices[bidx++];

			v1.x = 0;
			v1.y = 0;
			v1.z = 0;

			v2.x = x1;
			v2.y = 0;
			v2.z = z1;

			v3.x = x2;
			v3.y = 0;
			v3.z = z2;
		}
	}

	for (size_t i = 0; i < idx; i++)
	{
		auto& v = vertices[i];
		std::cout << "float3(" << v.x << ", " << v.y << ", " << v.z << "),\n";
	}

	for (size_t i = 0; i < bidx; i++)
	{
		auto& v = bvertices[i];
		std::cout << "float3(" << v.x << ", " << v.y << ", " << v.z << "),\n";
	}*/

	auto graphics = Graphics::Get();

	auto& renderer = *this;
	renderer.backCount = 0;
	renderer.buffers.resize(1);

	GRAPHICS_PIPELINE_DESC desc = {};
	desc.preferRenderCallPerFrame = 512;
	desc.vs = "DebugGraphics/Pyramid.vs";
	desc.ps = "DebugGraphics/Px.ps";
	desc.inputDesc.numElements = 0;
	desc.outputDesc.numRenderTarget = 1;
	desc.outputDesc.RTVFormat[0] = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	desc.outputDesc.DSVFormat = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;
	desc.rasterizerDesc.cullMode = GRAPHICS_CULL_MODE::NONE;
	desc.rasterizerDesc.fillMode = GRAPHICS_FILL_MODE::SOLID;

	renderer.pipeline = graphics->CreateRasterizerPipeline(desc);

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc = {};
	cbDesc.bufferSize = sizeof(CubeRenderer::CBuffer);
	cbDesc.perferNumRoom = 128;
	graphics->CreateConstantBuffers(1, &cbDesc, &renderer.constantBuffer);
}

void DebugGraphics::PyramidRenderer::Render(SharedPtr<GraphicsConstantBuffer>& cameraBuffer)
{
	auto graphics = Graphics::Get();

	graphics->SetGraphicsPipeline(pipeline.get());

	for (auto& buffer : buffers)
	{
		uint32_t instanceCount = PyramidRenderer::BATCH_SIZE;
		if (&buffer == &buffers.back())
		{
			instanceCount = backCount;
		}

		if (instanceCount == 0)
		{
			continue;
		}

		constantBuffer->UpdateBuffer(&buffer.transforms[0], sizeof(PyramidRenderer::CBuffer));

		auto params = pipeline->PrepareRenderParams();
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 0, 1, &cameraBuffer);
		params->SetConstantBuffers(GRAPHICS_SHADER_SPACE::SHADER_SPACE_VS, 1, 1, &constantBuffer);

		graphics->DrawInstanced(0, 0, 60, instanceCount, 0, 0);
	}

	buffers.resize(1);
	backCount = 0;
}

DebugGraphics::DebugGraphics()
{
	InitCubeRenderer();
}

void DebugGraphics::InitCubeRenderer()
{
	m_wireFrameCubeRenderer.Init(true);
	m_solidCubeRenderer.Init(false);
	m_pyramidRenderer.Init();
}

void DebugGraphics::DrawDirection(const Vec3& origin, const Vec3& direction, const Vec4& headColor, const Vec4& tailColor)
{
	auto dir = direction.Normal();
	Quaternion quat = Quaternion::RotationFromTo(Vec3::UP, dir);

	auto rot = Mat4::Rotation(quat);

	Mat4 transform = Mat4::Identity();
	transform *= rot;
	transform *= Mat4::Translation(origin + direction - dir * 0.2f);

	auto& renderer = m_pyramidRenderer;

	renderer.lock.lock();

	auto idx = renderer.backCount++;
	if (idx == CubeRenderer::BATCH_SIZE)
	{
		idx = 0;
		renderer.backCount = 1;
		renderer.buffers.emplace_back();
	}

	auto& back = renderer.buffers.back();
	back.transforms[idx] = transform;
	back.colors[idx] = headColor;

	auto d = direction.Length() / 2.0f - 0.2f / 2.0f;
	Mat4 cubeTransform = Mat4::Identity();
	cubeTransform *= Mat4::Scaling(0.02f, d, 0.02f);
	cubeTransform *= Mat4::Translation(Vec3::UP * d);
	cubeTransform *= rot;
	cubeTransform *= Mat4::Translation(origin);
	DrawCube(cubeTransform, tailColor);

	renderer.lock.unlock();
}

void DebugGraphics::DrawAABox(const AABox& aaBox, const Vec4& color)
{
	auto center = aaBox.GetCenter();
	auto halfDims = aaBox.GetHalfDimensions();

	Mat4 transform = Mat4::Identity();
	transform *= Mat4::Scaling(halfDims);
	transform *= Mat4::Translation(center);

	auto& renderer = m_wireFrameCubeRenderer;

	renderer.lock.lock();

	auto idx = renderer.backCount++;
	if (idx == CubeRenderer::BATCH_SIZE)
	{
		idx = 0;
		renderer.backCount = 1;
		renderer.buffers.emplace_back();
	}

	auto& back = renderer.buffers.back();
	back.transforms[idx] = transform;
	back.colors[idx] = color;

	renderer.lock.unlock();
}

void DebugGraphics::DrawCube(const Mat4& transform, const Vec4& color)
{
	auto& renderer = m_solidCubeRenderer;

	renderer.lock.lock();

	auto idx = renderer.backCount++;
	if (idx == CubeRenderer::BATCH_SIZE)
	{
		idx = 0;
		renderer.backCount = 1;
		renderer.buffers.emplace_back();
	}

	auto& back = renderer.buffers.back();
	back.transforms[idx] = transform;
	back.colors[idx] = color;

	renderer.lock.unlock();
}

void DebugGraphics::DrawFrustum(const Frustum& frustum, const Vec4& color)
{
}

void DebugGraphics::RenderToTarget(GraphicsRenderTarget* renderTarget, GraphicsDepthStencilBuffer* depthBuffer, 
	SharedPtr<GraphicsConstantBuffer>& cameraBuffer)
{
	auto graphics = Graphics::Get();
	graphics->SetRenderTargets(1, &renderTarget, depthBuffer);

	// render all cubes
	{
		m_wireFrameCubeRenderer.Render(cameraBuffer);
	}

	{
		m_solidCubeRenderer.Render(cameraBuffer);
	}

	{
		m_pyramidRenderer.Render(cameraBuffer);
	}

	graphics->UnsetRenderTargets(1, &renderTarget, depthBuffer);
}

NAMESPACE_END