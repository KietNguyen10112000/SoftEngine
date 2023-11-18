#include "Camera.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Rendering/CAMERA_PRIORITY.h"

#include "../RenderingPipeline/BasicRenderingPipeline.h"

NAMESPACE_BEGIN

Camera::Camera() : BaseCamera(RENDER_TYPE::RENDER_TYPE_CAMERA)
{
	// create render target for this camera
	GRAPHICS_RENDER_TARGET_DESC desc = {};
	desc.format = GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	desc.width = -1;
	desc.height = -1;
	desc.mipLevels = 1;
	Graphics::Get()->CreateRenderTargets(1, &desc, &m_renderTarget);

	GRAPHICS_SHADER_RESOURCE_DESC outputDesc = {};
	m_renderTarget->GetShaderResource()->GetDesc(&outputDesc);

	// create depth buffer
	GRAPHICS_DEPTH_STENCIL_BUFFER_DESC depthBufferDesc = {};
	depthBufferDesc.format = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;
	depthBufferDesc.mipLevels = 1;
	depthBufferDesc.width = outputDesc.texture2D.width;
	depthBufferDesc.height = outputDesc.texture2D.height;
	m_depthBuffer = Graphics::Get()->CreateDepthStencilBuffer(depthBufferDesc);

	// create rendering pipeline
	m_pipeline = new BasicRenderingPipeline();
}

//Camera::~Camera()
//{
//	CleanUp();
//}

void Camera::Serialize(Serializer* serializer)
{
}

void Camera::Deserialize(Serializer* serializer)
{
}

void Camera::CleanUp()
{
	delete m_pipeline;
	m_pipeline = nullptr;
}

Handle<ClassMetadata> Camera::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("Camera", this);

	auto accessor = Accessor(
		"Projection",
		&Projection(),
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{

		},

		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto& mat = var.As<Mat4>();
			auto ret = Variant(VARIANT_TYPE::PROJECTION_MAT4);
			ret.As<Mat4>() = mat;
			return ret;
		},
		this
	);

	metadata->AddProperty(accessor);

	return metadata;
}

void Camera::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	if (var.Is(&Projection()))
	{
		SetProjection(newValue.As<Mat4>());
	}
}

void Camera::OnComponentAdded()
{
	auto rdrSys = GetGameObject()->GetScene()->GetRenderingSystem();
	rdrSys->AddCamera(this, CAMERA_PRIORITY_LOWEST);
	rdrSys->DisplayCamera(this, rdrSys->GetDefaultViewport());

	/*static int count = 0;

	if (count++)
	{
		auto vp = rdrSys->GetDefaultViewport();
		vp.size /= 2.0f;
		rdrSys->DisplayCamera(this, vp);
	}
	else
	{
		auto vp = rdrSys->GetDefaultViewport();
		vp.size /= 2.0f;
		vp.topLeft += vp.size;
		rdrSys->DisplayCamera(this, vp);
	}*/
}

void Camera::OnComponentRemoved()
{
	auto rdrSys = GetGameObject()->GetScene()->GetRenderingSystem();
	rdrSys->RemoveCamera(this);
}

AABox Camera::GetGlobalAABB()
{
	return AABox();
}

void Camera::SetProjection(const Mat4& projMat)
{
	struct SetPerspectiveParam
	{
		Camera* camera;
		Mat4 projMat;
	};

	if (!GetGameObject()->IsInAnyScene())
	{
		Projection() = projMat;
		return;
	}

	auto system = GetGameObject()->GetScene()->GetRenderingSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](RenderingSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(SetPerspectiveParam, p, camera, projMat);
			camera->Projection() = projMat;
		}
	);

	auto param = taskRunner->CreateParam<SetPerspectiveParam>(&task);
	param->camera = this;
	param->projMat = projMat;

	taskRunner->RunAsync(&task);
}

NAMESPACE_END