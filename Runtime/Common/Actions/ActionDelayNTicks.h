#pragma once

#include "ActionBase.h"

NAMESPACE_BEGIN

class API ActionDelayNTicks : public ActionBase
{
protected:
	size_t m_remainTicks = 0;

public:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

public:
	static SharedPtr<ActionDelayNTicks> New(size_t NTicks);

};

NAMESPACE_END