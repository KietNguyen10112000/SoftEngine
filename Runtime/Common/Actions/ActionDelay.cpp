#include "ActionDelay.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionDelay::Update(float dt)
{
	m_remainTime -= dt;
	if (m_remainTime <= 0)
	{
		return RETURN_CODE::FINISHED;
	}

	return RETURN_CODE::NONE;
}

SharedPtr<ActionDelay> ActionDelay::New(float sec)
{
	auto ret = MakeShared<ActionDelay>();
	ret->m_remainTime = sec;
	return ret;
}

NAMESPACE_END