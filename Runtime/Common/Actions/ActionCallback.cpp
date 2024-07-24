#include "ActionCallback.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionCallback::Update(float dt)
{
	m_callback();
	return RETURN_CODE::FINISHED;
}

SharedPtr<ActionCallback> ActionCallback::New(const std::function<void()>& callback)
{
	auto ret = MakeShared<ActionCallback>();
	ret->m_callback = std::move(callback);
	return ret;
}

NAMESPACE_END