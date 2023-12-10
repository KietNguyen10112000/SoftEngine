#pragma once

#include "MeshBasicRenderer.h"

#include "Scene/DeferredBuffer.h"

NAMESPACE_BEGIN

class API AnimModelStaticMeshRenderer : public MeshBasicRenderer
{
public:
	friend class RenderingSystem;

	DeferredBuffer<Mat4> m_myGlobalTransform;

public:
	COMPONENT_CLASS(AnimModelStaticMeshRenderer);

	AnimModelStaticMeshRenderer(bool loadDefault = true);
	AnimModelStaticMeshRenderer(String modelPath, String texture2DPath);

	virtual void OnTransformChanged() override;

	virtual Handle<Serializable> Clone(Serializer* serializer) override;

};

NAMESPACE_END