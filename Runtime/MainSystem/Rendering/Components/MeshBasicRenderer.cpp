#include "MeshBasicRenderer.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

MeshBasicRenderer::MeshBasicRenderer(bool loadDefault) : RenderingComponent(RENDER_TYPE_MESH_BASIC_RENDERER)
{
	if (loadDefault)
	{
		m_model3D	= resource::Load<Model3DBasic>("Default/cube1.obj");
		m_mesh		= &m_model3D->m_meshes[0];
		m_texture	= resource::Load<Texture2D>("Default/default.png");
	}
}

MeshBasicRenderer::MeshBasicRenderer(String modelPath, String texture2DPath) : RenderingComponent(RENDER_TYPE_MESH_BASIC_RENDERER)
{
	m_model3D	= resource::Load<Model3DBasic>("Default/cube1.obj");
	m_mesh		= &m_model3D->m_meshes[0];
	m_texture	= resource::Load<Texture2D>(texture2DPath);
}

void MeshBasicRenderer::Serialize(Serializer* serializer)
{
}

void MeshBasicRenderer::Deserialize(Serializer* serializer)
{
}

void MeshBasicRenderer::CleanUp()
{
}

Handle<ClassMetadata> MeshBasicRenderer::GetMetadata(size_t sign)
{
	auto metadata = ClassMetadata::For(this);

	metadata->AddProperty(
		Accessor(
			"Model path",
			1,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{

			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto modelRenderer = (MeshBasicRenderer*)instance;
				Variant ret = Variant(VARIANT_TYPE::STRING_PATH);
				ret.As<String>() = modelRenderer->m_model3D->GetPath();
				return ret;
			},
			this
		)
	);

	metadata->AddProperty(
		Accessor(
			"Texture path",
			2,
			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{

			},
			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				auto modelRenderer = (MeshBasicRenderer*)instance;
				Variant ret = Variant(VARIANT_TYPE::STRING_PATH);
				ret.As<String>() = modelRenderer->m_texture->GetPath();
				return ret;
			},
			this
		)
	);

	return metadata;
}

void MeshBasicRenderer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	if (var.Is(1))
	{
		SetModel3D(newValue.AsString());
	}

	if (var.Is(2))
	{
		SetTexture(newValue.AsString());
	}
}

Handle<Serializable> MeshBasicRenderer::Clone(Serializer* serializer)
{
	auto ret = mheap::New<MeshBasicRenderer>(false);

	ret->m_model3D = m_model3D;
	ret->m_mesh = m_mesh;
	ret->m_texture = m_texture;

	return ret;
}

void MeshBasicRenderer::OnComponentAdded()
{
}

void MeshBasicRenderer::OnComponentRemoved()
{
}

AABox MeshBasicRenderer::GetGlobalAABB()
{
	auto localAABB = m_mesh->GetLocalAABB();
	localAABB.Transform(m_globalTransform);
	return localAABB;
}

void MeshBasicRenderer::SetModel3D(String path)
{
	struct SetModel3DParam
	{
		MeshBasicRenderer* model;
		String path;
	};

	std::string_view str = path.c_str();
	auto ext = str.substr(str.find_last_of('.') + 1);
	if (!(	ext == "obj"
		||	ext == "fbx"
		||	ext == "stl"
		||	ext == "dae"
		||	ext == "xml"
	)) {
		std::cerr << "MeshBasicRenderer::SetModel3D(): Unsupported format\n";
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		m_model3D = resource::Load<Model3DBasic>(path);
		m_mesh = &m_model3D->m_meshes[0];
		return;
	}

	auto system = GetGameObject()->GetScene()->GetRenderingSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](RenderingSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(SetModel3DParam, p, model, path);
			model->m_model3D = resource::Load<Model3DBasic>(path);
			model->m_mesh = &model->m_model3D->m_meshes[0];
		}
	);

	auto param = taskRunner->CreateParam<SetModel3DParam>(&task);
	param->model = this;
	param->path = path;

	taskRunner->RunAsync(&task);
}

void MeshBasicRenderer::SetTexture(String path)
{
	struct SetTextureParam
	{
		MeshBasicRenderer* model;
		String path;
	};

	std::string_view str = path.c_str();
	auto ext = str.substr(str.find_last_of('.') + 1);
	if (!(	ext == "png"
		||	ext == "jpg"
		||	ext == "bmp"
		||	ext == "jpeg"
	)) {
		std::cerr << "MeshBasicRenderer::SetTexture(): Unsupported format\n";
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		m_texture = resource::Load<Texture2D>(path);
		return;
	}

	auto system = GetGameObject()->GetScene()->GetRenderingSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](RenderingSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(SetTextureParam, p, model, path);
			model->m_texture = resource::Load<Texture2D>(path);
		}
	);

	auto param = taskRunner->CreateParam<SetTextureParam>(&task);
	param->model = this;
	param->path = path;

	taskRunner->RunAsync(&task);
}

NAMESPACE_END