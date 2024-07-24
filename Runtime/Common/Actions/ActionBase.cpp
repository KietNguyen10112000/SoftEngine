#include "ActionBase.h"

NAMESPACE_BEGIN

void ActionCompound::AddAction(const SharedPtr<ActionBase>& action)
{
	assert(action->m_next == nullptr && action->m_storageId == INVALID_ID && "action added twices");

	action->m_storageId = m_storage.size();
	m_storage.push_back(action);

	if (m_tail == nullptr && m_head == nullptr)
	{
		m_head = action.get();
		m_tail = action.get();
		return;
	}

	m_tail->m_next = action.get();
	action->m_prev = m_tail;
	m_tail = action.get();

	assert((m_tail == nullptr && m_head == nullptr) || (m_tail != nullptr && m_head != nullptr));
}

void ActionCompound::RemoveAction(ActionBase* action)
{
	auto& prev = action->m_prev;
	auto& next = action->m_next;

	assert(action->m_storageId != INVALID_ID && "action wasn't added");

	if (prev)
	{
		prev->m_next = next;
	}

	if (next)
	{
		next->m_prev = prev;
	}

	if (action == m_head)
	{
		m_head = next;
	}

	if (action == m_tail)
	{
		m_tail = prev;
	}

	assert((m_tail == nullptr && m_head == nullptr) || (m_tail != nullptr && m_head != nullptr));

	prev = nullptr;
	next = nullptr;
	assert(m_storage[action->m_storageId].get() == action);
	STD_VECTOR_ROLL_TO_FILL_BLANK(m_storage, action, m_storageId);
	action->m_storageId = INVALID_ID;
}

void ActionCompound::Clear()
{
	ForEachAction(
		[](ActionBase* action)
		{
			action->m_prev = nullptr;
			action->m_next = nullptr;
			action->m_storageId = INVALID_ID;
		}
	);

	m_head = nullptr;
	m_tail = nullptr;
	m_storage.clear();
}

bool ActionCompound::Contains(ActionBase* action) const
{
	return action->m_storageId != INVALID_ID && m_storage[action->m_storageId].get() == action;
}

NAMESPACE_END