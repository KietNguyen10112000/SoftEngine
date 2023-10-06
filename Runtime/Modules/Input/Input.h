#pragma once

#include "Core/TypeDef.h"
#include "Core/Time/Clock.h"
#include "KEYBOARD.h"

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
		Position offset;
		bool isActive = true;
		bool isLocked = true;

		Position minPos = { -INT_MAX, INT_MAX };
		Position maxPos = { -INT_MAX, INT_MAX };
		size_t numClampPos = 0;
	};

protected:
	constexpr static size_t NUM_KEYS = 256;
	constexpr static size_t NUM_CURSORS = 2;

	// ms
	size_t m_currentTime = 0;
	bool m_isKeysDownInFrame[NUM_KEYS] = {};
	bool m_isKeysUpInFrame[NUM_KEYS] = {};

	uint64_t m_keysDownCount[NUM_KEYS] = {};
	uint64_t m_keysUpCount[NUM_KEYS] = {};

	uint64_t m_lastKeysDownTime[NUM_KEYS] = {};

	Cursor m_prevCursors[NUM_CURSORS] = {};
	Cursor m_curCursors[NUM_CURSORS] = {};

	int m_windowWidth = 0;
	int m_windowHeight = 0;

protected:
	inline void DownKey(byte keyCode)
	{
		m_isKeysDownInFrame[keyCode] = true;

		if (m_keysDownCount[keyCode] > m_keysUpCount[keyCode])
		{
			return;
		}

		m_keysDownCount[keyCode]++;

		m_lastKeysDownTime[keyCode] = m_currentTime;
	}

	inline void UpKey(byte keyCode)
	{
		if (m_keysDownCount[keyCode] == m_keysUpCount[keyCode])
		{
			return;
		}

		m_isKeysUpInFrame[keyCode] = true;
		m_keysUpCount[keyCode]++;
	}

	inline void SetCursor(int id, int x, int y, int active)
	{
		auto& cursor = m_curCursors[id];
		auto& prevCursor = m_prevCursors[id];

		x = std::max(std::min(x, cursor.maxPos.x), cursor.minPos.x);
		y = std::max(std::min(y, cursor.maxPos.y), cursor.minPos.y);

		//if (cursor.isLocked)
		//{
		//	cursor.offset.x = x - prevCursor.position.x;
		//	cursor.offset.y = y - prevCursor.position.y;
		//	//return;
		//}

		cursor.offset.x = x - prevCursor.position.x;
		cursor.offset.y = y - prevCursor.position.y;

		/*if (cursor.offset.x != 0)
			cursor.offset.x = cursor.offset.x > 0 ? 1 : -1;

		if (cursor.offset.y != 0)
			cursor.offset.y = cursor.offset.y > 0 ? 1 : -1;*/

		cursor.position.x = x;
		cursor.position.y = y;
	}

public:
	inline void RollEvent()
	{
		m_currentTime = Clock::ms::now();

		::memset(m_isKeysDownInFrame, 0, sizeof(m_isKeysDownInFrame));
		::memset(m_isKeysUpInFrame, 0, sizeof(m_isKeysUpInFrame));

		::memcpy(m_prevCursors, m_curCursors, sizeof(Cursor) * NUM_CURSORS);
		
		for (auto& c : m_curCursors)
		{
			c.offset = { 0,0 };
			c.position = { 0,0 };
		}
	}

	inline bool IsKeyDown(byte keyCode)
	{
		return m_isKeysDownInFrame[keyCode] || m_keysDownCount[keyCode] > m_keysUpCount[keyCode];
	}

	inline bool IsKeyUp(byte keyCode)
	{
		return m_isKeysUpInFrame[keyCode] || m_keysDownCount[keyCode] == m_keysUpCount[keyCode];
	}

	// pressDuration in ms
	inline bool IsKeyPressed(byte keyCode, size_t pressDuration = 200)
	{
		return IsKeyUp(keyCode) && ((m_currentTime - m_lastKeysDownTime[keyCode]) < pressDuration);
	}

public:
	inline const auto& GetCursor(int id = 0)
	{
		return m_curCursors[id];
	}

	inline Position& GetDeltaCursorPosition(int id = 0)
	{
		//auto& prev = m_prevCursors[id];
		auto& cur = m_curCursors[id];

		return cur.offset;
	}

	inline bool IsCursorMoved(int id = 0)
	{
		auto& cur = m_curCursors[id];
		return cur.offset.x != 0 || cur.offset.y != 0;
	}

	inline void SetCursorLock(bool lock, int id = 0)
	{
		m_curCursors[id].isLocked = lock;
	}

	inline bool GetCursorLock(int id = 0)
	{
		return m_curCursors[id].isLocked;
	}

	inline void ClampCursor(int minX, int maxX, int minY, int maxY, int id = 0)
	{
		auto& cur = m_curCursors[id];
		cur.minPos.x = minX;
		cur.minPos.y = minY;
		cur.maxPos.x = maxX;
		cur.maxPos.y = maxY;
		cur.numClampPos++;
	}

	inline void SetClampCursorInsideWindow(bool clamp, int gapX = 30, int gapY = 30, int id = 0)
	{
		if (clamp)
		{
			ClampCursor(gapX, GetWindowWidth() - gapX, gapY, GetWindowHeight() - gapY, id);
		}
		else
		{
			ClampCursor(-INT_MAX, INT_MAX, -INT_MAX, INT_MAX, id);
		}
	}

	inline int GetWindowWidth()
	{
		return m_windowWidth;
	}

	inline int GetWindowHeight()
	{
		return m_windowHeight;
	}
};

NAMESPACE_END