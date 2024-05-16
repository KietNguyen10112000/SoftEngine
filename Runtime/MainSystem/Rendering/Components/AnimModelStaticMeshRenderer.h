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

	//virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override
	{
		auto localAABB = m_mesh->GetLocalAABB();
		localAABB.Transform(GetGlobalTransform());
		return localAABB;
	}

protected:
	virtual void CloneFrom(Serializer* serializer, Serializable* another) override;

public:
	inline Mat4 GetGlobalTransform()
	{
		return (*m_myGlobalTransform.Read()) * GetGameObject()->GetCommittedGlobalTransform();
	}

};

NAMESPACE_END