#pragma once

#include "RenderingComponent.h"
#include "Resources/Model3DBasic.h"
#include "Resources/Texture2D.h"

NAMESPACE_BEGIN

class Model3DBasicRenderer : public RenderingComponent
{
public:
	friend class RenderingSystem;

	Resource<Model3DBasic>	m_model;
	Resource<Texture2D>		m_texture;

public:
	COMPONENT_CLASS(Model3DBasicRenderer);

	Model3DBasicRenderer();
	Model3DBasicRenderer(String modelPath, String texture2DPath);

	// Inherited via RenderingComponent
	virtual void Serialize(Serializer* serializer) override;

	virtual void Deserialize(Serializer* serializer) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual AABox GetGlobalAABB() override;

	inline auto& GetModel()
	{
		return m_model;
	}

	inline auto& GetTexture2D()
	{
		return m_texture;
	}
};

NAMESPACE_END