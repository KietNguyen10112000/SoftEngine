#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/SmartPointers.h"

#include "Core/Structures/STD/STDContainers.h"

//#include <vector>

NAMESPACE_BEGIN

class ActionBase
{
public:
#ifdef ERROR
#undef ERROR
#endif // ERROR

	enum class RETURN_CODE
	{
		NONE,
		ERROR,
		FINISHED
	};

private:
	friend class ActionCompound;

	ActionBase* m_prev = nullptr;
	ActionBase* m_next = nullptr;
	ID m_storageId = INVALID_ID;

public:
	virtual ~ActionBase() {};
	virtual RETURN_CODE Update(float dt) = 0;

	static bool HasLoop(ActionBase* action);

};

// action that contains others must be extended from class ActionCompound, don't use your approach which can lead to memory leak
class API ActionCompound : public ActionBase
{
private:
	friend class ActionBase;

	ActionBase* m_head = nullptr;
	ActionBase* m_tail = nullptr;
	std::vector<SharedPtr<ActionBase>> m_storage;

protected:
	void AddAction(const SharedPtr<ActionBase>& action);
	void RemoveAction(ActionBase* action);
	void Clear();

	template <typename Fn>
	inline void ForEachAction(Fn callback)
	{
		auto it = m_head;
		while (it)
		{
			auto next = it->m_next;
			callback(it);
			it = next;
		}
	}

	inline ActionBase* Front()
	{
		return m_head;
	}

	inline ActionBase* Back()
	{
		return m_head;
	}

public:
	bool Contains(ActionBase* action) const;

	inline bool Contains(const SharedPtr<ActionBase>& action) const
	{
		return Contains(action.get());
	}

};

NAMESPACE_END