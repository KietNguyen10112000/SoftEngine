#pragma once

#include "Rendering.h"

#include "Math/Math.h"

#include "Objects/GameObject.h"
#include "Objects/Scene/Scene.h"
#include "SubSystems/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

class Camera : public Rendering
{
protected:
	Mat4 m_proj;

public:
	inline Camera(float fovY, float aspectRatio, float dNear, float dFar)
	{
		m_proj.SetPerspectiveFovLH(fovY, aspectRatio, dNear, dFar);
	};

public:
	virtual void OnComponentAddedToScene() final override
	{
		m_object->GetScene()->GetRenderingSystem()->AddCamera(m_object);
	}

	virtual void OnComponentAdded() override {}

	virtual void OnComponentRemoved() override {}

	virtual void OnComponentRemovedFromScene() override {}

	virtual void SetAsMain() override {}

	virtual void SetAsExtra() override {}

	virtual void ResolveBranch() override {}

	virtual bool IsNewBranch() override { return false; }

public:
	inline Mat4 GetView()
	{
		return GetObject()->GetTransformMat4().GetInverse();
	}

	inline auto& GetProj() const
	{
		return m_proj;
	}

};

NAMESPACE_END