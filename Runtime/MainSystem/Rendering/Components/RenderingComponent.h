#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "RENDER_TYPE.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class RenderingComponent : public MainComponent
{
private:
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::RENDERING_ID;

protected:
	const RENDER_TYPE m_RENDER_TYPE;

	Mat4 m_globalTransform;

public:
	RenderingComponent(const RENDER_TYPE type) : m_RENDER_TYPE(type) {};
	virtual ~RenderingComponent() {};

	virtual void OnTransformChanged() override;

	inline auto GetRenderType() const
	{
		return m_RENDER_TYPE;
	}

	inline auto& GlobalTransform() const
	{
		return m_globalTransform;
	}

};

NAMESPACE_END