#pragma once

#include "ActionBase.h"

NAMESPACE_BEGIN

class API ActionDelay : public ActionBase
{
protected:
	float m_remainTime = 0.0f;

public:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

public:
	static SharedPtr<ActionDelay> New(float sec);

};

NAMESPACE_END