#pragma once

#include "MeshBasicRenderer.h"

#include "Scene/DeferredBuffer.h"

NAMESPACE_BEGIN

class API AnimModelStaticMeshRenderer : public MeshBasicRenderer
{
public:
	friend class RenderingSystem;
	friend class AnimatorSkeletalArray;

	DeferredBuffer<Mat4> m_myGlobalTransform;

	bool m_discardObjectTransform = false;

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
		return m_discardObjectTransform ? (*m_myGlobalTransform.Read()) : (*m_myGlobalTransform.Read()) * GetGameObject()->GetCommittedGlobalTransform();
	}

};

NAMESPACE_END