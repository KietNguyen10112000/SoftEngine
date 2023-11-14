#pragma once

#include "RenderingComponent.h"
#include "Resources/MeshBasic.h"
#include "Resources/Model3DBasic.h"
#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class API MeshBasicRenderer : public RenderingComponent
{
public:
	friend class RenderingSystem;

	Resource<Model3DBasic>	m_model3D;
	Resource<MeshBasic>		m_mesh;
	Resource<Texture2D>		m_texture;

public:
	COMPONENT_CLASS(MeshBasicRenderer);

	MeshBasicRenderer(bool loadDefault = true);
	MeshBasicRenderer(String modelPath, String texture2DPath);

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

public:
	void SetModel3D(String path);
	void SetTexture(String path);

};

NAMESPACE_END