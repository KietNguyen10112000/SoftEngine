#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "ANIMATION_TYPE.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class API AnimationComponent : public MainComponent
{
private:
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::RENDERING_ID;

protected:
	const ANIMATION_TYPE m_ANIMATION_TYPE;

public:
	AnimationComponent(const ANIMATION_TYPE type) : m_ANIMATION_TYPE(type) {};
	virtual ~AnimationComponent() {};

	inline auto GetAnimationType() const
	{
		return m_ANIMATION_TYPE;
	}

};

NAMESPACE_END