#include "AnimMeshRenderer.h"

NAMESPACE_BEGIN

AnimMeshRenderer::AnimMeshRenderer() : RenderingComponent(RENDER_TYPE_ANIM_MESH_RENDERER)
{
}

Handle<ClassMetadata> AnimMeshRenderer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimMeshRenderer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void AnimMeshRenderer::CloneFrom(Serializer* serializer, Serializable* another)
{
	RenderingComponent::CloneFrom(serializer, another);

	auto src = (AnimMeshRenderer*)another;

	m_model3D = src->m_model3D;
	m_mesh = src->m_mesh;
	m_texture = src->m_texture;

	m_animMeshRenderingBuffer = serializer->Clone(src->m_animMeshRenderingBuffer);
}

void AnimMeshRenderer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimMeshRenderer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimMeshRenderer::SerializeToJson(Serializer* serializer, json& j) const
{
	RenderingComponent::SerializeToJson(serializer, j);

	j["Model"]		= serializer->Serialize(m_model3D);
	j["MeshId"]		= m_mesh->m_model3DIdx;
	j["Texture2D"]	= serializer->Serialize(m_texture);
	j["AnimMeshRenderingBuffer"] = serializer->Serialize(m_animMeshRenderingBuffer);
}

void AnimMeshRenderer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	RenderingComponent::DeserializeFromJson(serializer, j);

	serializer->Deserialize(j["Model"], m_model3D);
	m_mesh = &m_model3D->m_animMeshes[uint32_t(j["MeshId"])];
	serializer->Deserialize(j["Texture2D"], m_texture);
	serializer->Deserialize(j["AnimMeshRenderingBuffer"], m_animMeshRenderingBuffer);
}

void AnimMeshRenderer::OnComponentAdded()
{
}

void AnimMeshRenderer::OnComponentRemoved()
{
}

AABox AnimMeshRenderer::GetGlobalAABB()
{
	return m_animMeshRenderingBuffer->discardObjectTransform ?
		m_animMeshRenderingBuffer->buffer.Read()->meshesAABB[m_mesh->m_model3DIdx]:
		m_animMeshRenderingBuffer->buffer.Read()->meshesAABB[m_mesh->m_model3DIdx].MakeTransform(GlobalTransform());
	//return AABox(Vec3(0,0,0), Vec3(10000,10000,10000));
}

NAMESPACE_END