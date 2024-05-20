#include "Camera.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Rendering/CAMERA_PRIORITY.h"

#include "../RenderingPipeline/BasicRenderingPipeline.h"

NAMESPACE_BEGIN

void BaseCamera::Init(int renderWidth, int renderHeight)
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

Camera::Camera() : BaseCamera(RENDER_TYPE::RENDER_TYPE_CAMERA)
{
	Init(-1, -1);
}

//Camera::~Camera()
//{
//	CleanUp();
//}

void Camera::OnComponentAdded()
{
	auto rdrSys = GetGameObject()->GetScene()->GetRenderingSystem();
	rdrSys->AddCamera(this, CAMERA_PRIORITY_LOWEST);

	if (!IsDisplaying())
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
			TASK_SYSTEM_UNPACK_PARAM_REF_2(SetPerspectiveParam, p, camera, projMat);
			camera->Projection() = projMat;
		}
	);

	auto param = taskRunner->CreateParam<SetPerspectiveParam>(&task);
	param->camera = this;
	param->projMat = projMat;

	taskRunner->RunAsync(&task);
}

void Camera::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void Camera::SerializeToJson(Serializer* serializer, json& j) const
{
	auto& projectionMat = m_proj;

	if (projectionMat[3][3])
	{
		// ortho
		j["IsPerspective"] = false;

		j["Width"] = 2.0f / projectionMat[0][0];
		j["Height"] = 2.0f / projectionMat[1][1];
		float near = -projectionMat[3][2] / projectionMat[2][2];
		j["Near"] = near;
		j["Far"] = (1.0f / projectionMat[2][2]) + near;
	}
	else
	{
		// perspective
		j["IsPerspective"] = true;

		j["FovY"] = 2.0f * std::atan(1 / projectionMat[1][1]);
		j["Aspect"] = projectionMat[1][1] / projectionMat[0][0];
		float near = -projectionMat[3][2] / projectionMat[2][2];
		j["Near"] = near;
		j["Far"] = near / (1.0f - 1.0f / projectionMat[2][2]);
	}
}

void Camera::DeserializeFromJson(Serializer* serializer, const json& j)
{
	//Init(-1, -1);

	bool isPerspective = j["IsPerspective"];
	if (!isPerspective)
	{
		Projection().SetOrthographicLH(j["Width"], j["Height"], j["Near"], j["Far"]);
	}
	else
	{
		Projection().SetPerspectiveFovLH(j["FovY"], j["Aspect"], j["Near"], j["Far"]);
	}
}

void Camera::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void Camera::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
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

NAMESPACE_END