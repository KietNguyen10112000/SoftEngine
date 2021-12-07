#pragma once

#include <IObject.h>

#include <Engine/Engine.h>

class ControllableCamera : public ICamera
{
public:
	inline virtual void UpdateControl(Engine* engine) = 0;
};