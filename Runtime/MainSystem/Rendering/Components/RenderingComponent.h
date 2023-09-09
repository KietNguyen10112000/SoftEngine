#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class RenderingComponent : public MainComponent
{
private:
	friend class GameObject;
	constexpr static ID COMPONENT_ID = MainSystemInfo::RENDERING_ID;

protected:
	Mat4 m_globalTransform;

public:
	virtual void Render() = 0;


};

NAMESPACE_END