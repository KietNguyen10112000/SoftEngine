#include "ActionRepeatUntil.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionRepeatUntil::Update(float dt)
{
	Front()->Update(dt);

	if (m_checker != nullptr && m_checker())
	{
		return RETURN_CODE::FINISHED;
	}

	return RETURN_CODE::NONE;
}

SharedPtr<ActionRepeatUntil> ActionRepeatUntil::New(const SharedPtr<ActionBase>& action, const std::function<bool()>& callback)
{
	auto ret = std::make_shared<ActionRepeatUntil>();
	ret->m_checker = callback;
	ret->AddAction(action);
	return ret;
}

NAMESPACE_END