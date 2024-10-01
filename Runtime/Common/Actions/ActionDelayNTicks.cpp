#include "ActionDelayNTicks.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionDelayNTicks::Update(float dt)
{
	m_remainTicks--;
	if (m_remainTicks <= 0)
	{
		return RETURN_CODE::FINISHED;
	}

	return RETURN_CODE::NONE;
}

SharedPtr<ActionDelayNTicks> ActionDelayNTicks::New(size_t NTicks)
{
	auto ret = MakeShared<ActionDelayNTicks>();
	ret->m_remainTicks = NTicks;
	return ret;
}

NAMESPACE_END