#pragma once

#include "Core/TypeDef.h"
#include "Core/Time/Clock.h"

NAMESPACE_BEGIN

class Input
{
public:
	struct Position
	{
		int x;
		int y;
	};

	struct Cursor
	{
		Position position;
		bool isActive = true;
		bool isLocked = true;
	};

private:
	constexpr static size_t NUM_KEYS = 256;
	constexpr static size_t NUM_CURSORS = 2;

	// ms
	size_t m_currentTime = 0;
	size_t m_keysDownTime[NUM_KEYS] = {};
	uint32_t m_keysDownCount[NUM_KEYS] = {};
	uint32_t m_keysUpCount[NUM_KEYS] = {};
	//size_t m_keysUpTime[NUM_KEYS] = {};

	//bool m_prevKeys[NUM_KEYS] = {};
	bool m_curKeys[NUM_KEYS] = {};

	Cursor m_prevCursors[NUM_CURSORS] = {};
	Cursor m_curCursors[NUM_CURSORS] = {};

protected:
	inline void DownKey(byte keyCode)
	{
		m_keysDownCount[keyCode]++;
		if (m_curKeys[keyCode] == false)
		{
			//m_prevKeys[keyCode] = m_curKeys[keyCode];
			m_keysDownTime[keyCode] = m_currentTime;
			m_curKeys[keyCode] = true;
		}
	}

	inline void UpKey(byte keyCode)
	{
		//m_prevKeys[keyCode] = m_curKeys[keyCode];
		m_keysUpCount[keyCode]++;
		m_curKeys[keyCode] = false;
	}

	inline void SetCursor(int id, int x, int y, int active)
	{
		auto& cursor = m_curCursors[id];
		auto& prevCursor = m_prevCursors[id];

		if (cursor.isLocked)
		{
			cursor.position.x = x - prevCursor.position.x;
			cursor.position.y = y - prevCursor.position.y;
			return;
		}

		cursor.position.x = x;
		cursor.position.y = y;
	}

public:
	inline void RollEvent()
	{
		m_currentTime = Clock::ms::now();
		::memset(m_keysDownCount, 0, sizeof(uint32_t) * NUM_KEYS);
		::memset(m_keysUpCount, 0, sizeof(uint32_t) * NUM_KEYS);
		//::memcpy(m_prevKeys, m_curKeys, sizeof(bool) * NUM_KEYS);
		::memcpy(m_prevCursors, m_curCursors, sizeof(Cursor) * NUM_CURSORS);
	}

	inline bool IsKeyDown(byte keyCode)
	{
		return m_curKeys[keyCode] || m_keysDownCount[keyCode] != 0;
	}

	inline bool IsKeyUp(byte keyCode)
	{
		return m_keysUpCount[keyCode] != 0;
	}

	// pressDuration in ms
	inline bool IsKeyPressed(byte keyCode, size_t pressDuration = 500)
	{
		return IsKeyUp(keyCode) && ((m_currentTime - m_keysDownTime[keyCode]) < pressDuration);
	}

public:
	inline const auto& GetCursor(int id = 0)
	{
		return m_curCursors[id];
	}

	inline Position GetDeltaCursorPosition(int id = 0)
	{
		auto& prev = m_prevCursors[id];
		auto& cur = m_curCursors[id];

		if (cur.isLocked)
		{
			return { cur.position.x, cur.position.y };
		}

		return { cur.position.x - prev.position.x, cur.position.y - prev.position.y };
	}

	inline void LockCursor(int id = 0)
	{
		m_curCursors[id].isLocked = true;
	}

	inline void UnlockCursor(int id = 0)
	{
		m_curCursors[id].isLocked = false;
	}
};

NAMESPACE_END