#pragma once

#include <Windows.h>

class CriticalSection
{
public:
	CRITICAL_SECTION m_criticalSection = {};

public:
	inline CriticalSection()
	{
		(void)InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x80000400);
	};

	inline ~CriticalSection()
	{
		DeleteCriticalSection(&m_criticalSection);
	};

public:
	inline void Enter()
	{
		EnterCriticalSection(&m_criticalSection);
	};

	inline void Leave()
	{
		LeaveCriticalSection(&m_criticalSection);
	};

};
