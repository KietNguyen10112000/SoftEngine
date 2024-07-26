#pragma once

#include "ActionBase.h"

#include <functional>

NAMESPACE_BEGIN

class ActionRepeatUntil : public ActionCompound
{
private:
	std::function<bool()> m_checker;

public:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

public:
	static SharedPtr<ActionRepeatUntil> New(const SharedPtr<ActionBase>& action, const std::function<bool()>& checker);

};

NAMESPACE_END