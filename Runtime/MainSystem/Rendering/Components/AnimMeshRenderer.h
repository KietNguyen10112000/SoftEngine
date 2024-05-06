#pragma once

#include "RenderingComponent.h"
#include "Resources/AnimModel.h"
#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class API AnimMeshRenderer : public RenderingComponent
{
public:
	friend class RenderingSystem;

	Resource<AnimModel>		m_model3D;
	AnimModel::AnimMesh*	m_mesh;
	Resource<Texture2D>		m_texture;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

public:
	COMPONENT_CLASS(AnimMeshRenderer);

	AnimMeshRenderer();

protected:
	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

public:
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual AABox GetGlobalAABB() override;

	inline auto& GetMesh()
	{
		return m_mesh;
	}

	inline auto& GetTexture2D()
	{
		return m_texture;
	}


};

NAMESPACE_END