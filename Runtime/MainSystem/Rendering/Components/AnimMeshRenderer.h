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

	// Inherited via RenderingComponent
	virtual void Serialize(Serializer* serializer) override;

	virtual void Deserialize(Serializer* serializer) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

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