#include "ActionSequence.h"

NAMESPACE_BEGIN

ActionBase::RETURN_CODE ActionSequence::Update(float dt)
{
	auto front = Front();
	while (front && front->Update(dt) == RETURN_CODE::FINISHED)
	{
		RemoveAction(front);
		front = Front();
	}

	if (Front() == nullptr)
	{
		return RETURN_CODE::FINISHED;
	}

	return RETURN_CODE::NONE;
}

void ActionSequence::RunAction(const SharedPtr<ActionBase>& action)
{
	AddAction(action);
}

void ActionSequence::RunActions(const std::vector<SharedPtr<ActionBase>>& actions)
{
	for (auto& a : actions)
	{
		AddAction(a);
	}
}

void ActionSequence::StopAction(ActionBase* action)
{
	if (Contains(action))
	{
		RemoveAction(action);
	}
}

void ActionSequence::StopAllActions()
{
	Clear();
}

SharedPtr<ActionSequence> ActionSequence::New(const std::vector<SharedPtr<ActionBase>>& actions)
{
	auto ret = MakeShared<ActionSequence>();
	ret->RunActions(actions);
	return ret;
}

NAMESPACE_END