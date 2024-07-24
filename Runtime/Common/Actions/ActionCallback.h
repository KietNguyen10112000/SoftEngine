#pragma once

#include "ActionBase.h"

#include <functional>

NAMESPACE_BEGIN

class API ActionCallback : public ActionBase
{
protected:
	std::function<void()> m_callback;

public:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

public:
	static SharedPtr<ActionCallback> New(const std::function<void()>& callback);

};

NAMESPACE_END