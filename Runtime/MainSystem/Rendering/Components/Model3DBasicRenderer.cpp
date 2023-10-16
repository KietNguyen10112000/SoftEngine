#include "Model3DBasicRenderer.h"

NAMESPACE_BEGIN

Model3DBasicRenderer::Model3DBasicRenderer() : RenderingComponent(RENDER_TYPE_MODEL3D_BASIC_RENDERER)
{
}

Model3DBasicRenderer::Model3DBasicRenderer(String modelPath, String texture2DPath) : RenderingComponent(RENDER_TYPE_MODEL3D_BASIC_RENDERER)
{
	m_model		= resource::Load<Model3DBasic>(modelPath);
	m_texture	= resource::Load<Texture2D>(texture2DPath);
}

void Model3DBasicRenderer::Serialize(ByteStream& stream)
{
}

void Model3DBasicRenderer::Deserialize(ByteStreamRead& stream)
{
}

void Model3DBasicRenderer::CleanUp()
{
}

Handle<ClassMetadata> Model3DBasicRenderer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Model3DBasicRenderer::OnPropertyChanged(const UnknownAddress& var)
{
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

NAMESPACE_END