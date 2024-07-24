#include "ActionExecution.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionExecution::Update(float dt)
{
	ForEachAction(
		[&](ActionBase* action)
		{
			if (action->Update(dt) == RETURN_CODE::FINISHED)
			{
				RemoveAction(action);
			}
		}
	);

	if (Front() == nullptr)
	{
		return RETURN_CODE::FINISHED;
	}

	return RETURN_CODE::NONE;
}

void ActionExecution::RunAction(const SharedPtr<ActionBase>& action)
{
	AddAction(action);
}

void ActionExecution::RunActions(const std::vector<SharedPtr<ActionBase>>& actions)
{
	for (auto& a : actions)
	{
		AddAction(a);
	}
}

void ActionExecution::StopAction(ActionBase* action)
{
	if (Contains(action))
	{
		RemoveAction(action);
	}
}

void ActionExecution::StopAllActions()
{
	Clear();
}

SharedPtr<ActionExecution> ActionExecution::New(const std::vector<SharedPtr<ActionBase>>& actions)
{
	auto ret = MakeShared<ActionExecution>();
	ret->RunActions(actions);
	return ret;
}

NAMESPACE_END