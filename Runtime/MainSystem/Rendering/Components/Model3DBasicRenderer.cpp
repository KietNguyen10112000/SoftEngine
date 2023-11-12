#include "Model3DBasicRenderer.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

Model3DBasicRenderer::Model3DBasicRenderer() : RenderingComponent(RENDER_TYPE_MODEL3D_BASIC_RENDERER)
{
	m_model = resource::Load<Model3DBasic>("Default/cube.obj");
	m_texture = resource::Load<Texture2D>("Default/default.png");
}

Model3DBasicRenderer::Model3DBasicRenderer(String modelPath, String texture2DPath) : RenderingComponent(RENDER_TYPE_MODEL3D_BASIC_RENDERER)
{
	m_model		= resource::Load<Model3DBasic>(modelPath);
	m_texture	= resource::Load<Texture2D>(texture2DPath);
}

void Model3DBasicRenderer::Serialize(Serializer* serializer)
{
}

void Model3DBasicRenderer::Deserialize(Serializer* serializer)
{
}

void Model3DBasicRenderer::CleanUp()
{
}

Handle<ClassMetadata> Model3DBasicRenderer::GetMetadata(size_t sign)
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
				auto modelRenderer = (Model3DBasicRenderer*)instance;
				Variant ret = Variant(VARIANT_TYPE::STRING_PATH);
				ret.As<String>() = modelRenderer->m_model->GetPath();
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
				auto modelRenderer = (Model3DBasicRenderer*)instance;
				Variant ret = Variant(VARIANT_TYPE::STRING_PATH);
				ret.As<String>() = modelRenderer->m_texture->GetPath();
				return ret;
			},
			this
		)
	);

	return metadata;
}

void Model3DBasicRenderer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
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

void Model3DBasicRenderer::OnComponentAdded()
{
}

void Model3DBasicRenderer::OnComponentRemoved()
{
}

AABox Model3DBasicRenderer::GetGlobalAABB()
{
	auto localAABB = m_model->GetLocalAABB();
	localAABB.Transform(m_globalTransform);
	return localAABB;
}

void Model3DBasicRenderer::SetModel3D(String path)
{
	struct SetModel3DParam
	{
		Model3DBasicRenderer* model;
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
		std::cerr << "Model3DBasicRenderer::SetModel3D(): Unsupported format\n";
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		m_model = resource::Load<Model3DBasic>(path);
		return;
	}

	auto system = GetGameObject()->GetScene()->GetRenderingSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](RenderingSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(SetModel3DParam, p, model, path);
			model->m_model = resource::Load<Model3DBasic>(path);
		}
	);

	auto param = taskRunner->CreateParam<SetModel3DParam>(&task);
	param->model = this;
	param->path = path;

	taskRunner->RunAsync(&task);
}

void Model3DBasicRenderer::SetTexture(String path)
{
	struct SetTextureParam
	{
		Model3DBasicRenderer* model;
		String path;
	};

	std::string_view str = path.c_str();
	auto ext = str.substr(str.find_last_of('.') + 1);
	if (!(	ext == "png"
		||	ext == "jpg"
		||	ext == "bmp"
		||	ext == "jpeg"
	)) {
		std::cerr << "Model3DBasicRenderer::SetTexture(): Unsupported format\n";
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