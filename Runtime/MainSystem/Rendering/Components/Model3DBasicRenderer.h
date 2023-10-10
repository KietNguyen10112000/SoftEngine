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
	Model3DBasicRenderer(String modelPath, String texture2DPath);

	// Inherited via RenderingComponent
	virtual void Serialize(ByteStream& stream) override;

	virtual void Deserialize(ByteStreamRead& stream) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var) override;

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