#include "AnimMeshRenderer.h"

NAMESPACE_BEGIN

AnimMeshRenderer::AnimMeshRenderer() : RenderingComponent(RENDER_TYPE_ANIM_MESH_RENDERER)
{
}

void AnimMeshRenderer::Serialize(Serializer* serializer)
{
}

void AnimMeshRenderer::Deserialize(Serializer* serializer)
{
}

void AnimMeshRenderer::CleanUp()
{
}

Handle<ClassMetadata> AnimMeshRenderer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimMeshRenderer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void AnimMeshRenderer::OnComponentAdded()
{
}

void AnimMeshRenderer::OnComponentRemoved()
{
}

AABox AnimMeshRenderer::GetGlobalAABB()
{
	return AABox();
}

NAMESPACE_END