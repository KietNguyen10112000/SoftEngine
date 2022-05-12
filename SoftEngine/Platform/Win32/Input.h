#pragma once
#include <Windows.h>

#define ESC 0x1B
#define TAB 0x09
#define L_SHIFT VK_SHIFT
#define R_SHIFT VK_SHIFT
#define L_CTRL VK_CONTROL
#define R_CTRL VK_CONTROL
#define ENTER 0x0D
#define SPACE 0x20
#define LEFT_ARROW 0x25
#define UP_ARROW 0x26
#define RIGHT_ARROW 0x27
#define DOWN_ARROW 0x28

enum MOUSE_BUTTON
{
	LEFT,
	RIGHT,
	MID,
	NONE
};

LRESULT WndHandle(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Input
{
private:
	friend LRESULT WndHandle(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	inline static constexpr size_t KEY_COUNT = 256;
	inline static constexpr size_t MOUSE_COUNT = 10;

private:
	inline static bool key[KEY_COUNT] = {};
	//static int lastKeyDown;
	inline static int lastKeyUp = 0;
	inline static bool mouse[MOUSE_COUNT] = {};
	inline static MOUSE_BUTTON doubleClick = NONE;
	inline static MOUSE_BUTTON click = NONE;
	inline static short deltaMouseWheel = 0;
	inline static float deltaClickTimer = 0;

private:
	LPPOINT curPoint = new tagPOINT();

	int m_pressKey = -1;
	int m_curMousePos[2] = { 0,0 };
	int m_preMousePos[2] = { 0,0 };
	int m_deltaMPos[2] = { 0,0 };

	bool m_lockMouse = false;
	int m_lockMousePosX = 0;
	int m_lockMousePosY = 0;

	bool m_hideCursor = false;

	bool m_preKey[256] = {};
	bool m_preMouse[10] = {};

public:
	inline Input() {};
	inline ~Input() {};

public:
	inline static float deltaClickTime = 0;

public:
	inline bool GetKeyPressed(unsigned char keyCode);
	inline bool GetMouseDBClicked(MOUSE_BUTTON button);
	inline bool GetMouseClicked(MOUSE_BUTTON button);
	inline void UpdateMousePos();
	inline void LastUpdate();
	inline void SetHideCursor(bool hide);
	inline bool IsHideCursor();
	inline bool IsMouseDrag() { return mouse[0] || mouse[1] || mouse[2]; }
	inline void SetLockMouse(bool lock, int posX, int posY);

	inline bool GetLockMouse() { return m_lockMouse; };
	inline bool GetMouseButton(MOUSE_BUTTON button) { return mouse[button]; };
	inline bool GetKey(unsigned char keyCode) { return key[keyCode]; };
	inline void FirstUpdate() { UpdateMousePos(); };
	inline int* GetMousePos() { return m_curMousePos; };
	inline int* GetPreMousePos() { return m_preMousePos; };
	inline int* GetDeltaMousePos() { return m_deltaMPos; };
	inline bool IsMouseMove() { return (m_deltaMPos[0] != 0 || m_deltaMPos[1] != 0); };
	inline short GetDeltaMouseHWheel() { return deltaMouseWheel; };

	inline bool GetKeyDown(unsigned char keyCode) { return m_preKey[keyCode] == false && key[keyCode] == true; };
	inline bool GetKeyUp(unsigned char keyCode) { return m_preKey[keyCode] == true && key[keyCode] == false; };
	inline bool GetMouseDown(MOUSE_BUTTON button) { return m_preMouse[button] == false && mouse[button] == true; };
	inline bool GetMouseUp(MOUSE_BUTTON button) { return m_preMouse[button] == true && mouse[button] == false; };

};

inline bool Input::GetKeyPressed(unsigned char keyCode)
{
	/*if (lastKeyDown != keyCode)
	{
		return false;
	}
	if (lastKeyUp == keyCode)
	{
		m_pressKey = -1;
		lastKeyUp = -1;
		return false;
	}

	if (key[keyCode] && m_pressKey != keyCode)
	{
		m_pressKey = keyCode;
		return true;
	}*/

	if (lastKeyUp == keyCode)
	{
		return true;
	}

	return false;
}

inline bool Input::GetMouseDBClicked(MOUSE_BUTTON button)
{
	if (button == doubleClick) return true;
	return false;
}

inline bool Input::GetMouseClicked(MOUSE_BUTTON button)
{
	if (button == click && !mouse[button])
	{
		return true;
	}
	return false;
}

inline void Input::SetLockMouse(bool lock, int posX, int posY)
{
	SetCursorPos(posX, posY);
	m_lockMouse = lock;
	m_lockMousePosX = posX;
	m_lockMousePosY = posY;
	m_curMousePos[0] = posX;
	m_curMousePos[1] = posY;
	m_preMousePos[0] = posX;
	m_preMousePos[1] = posY;
	m_deltaMPos[0] = 0;
	m_deltaMPos[1] = 0;
}

inline void Input::LastUpdate()
{
	doubleClick = MOUSE_BUTTON::NONE;
	if (deltaClickTimer > deltaClickTime)
	{
		click = NONE;
		deltaClickTimer = 0;
	}

	lastKeyUp = -1;
	deltaMouseWheel = 0;

	memcpy(m_preKey, key, KEY_COUNT * sizeof(bool));
	memcpy(m_preMouse, mouse, MOUSE_COUNT * sizeof(bool));
}

inline void Input::SetHideCursor(bool hide)
{
	m_hideCursor = hide;
	ShowCursor(!hide);
}

inline bool Input::IsHideCursor()
{
	return m_hideCursor;
}

inline void Input::UpdateMousePos()
{
	GetCursorPos(curPoint);
	if (m_lockMouse)
	{
		SetCursorPos(m_lockMousePosX, m_lockMousePosY);
		m_deltaMPos[0] = curPoint->x - m_lockMousePosX;
		m_deltaMPos[1] = curPoint->y - m_lockMousePosY;
	}
	else
	{
		m_preMousePos[0] = m_curMousePos[0];
		m_preMousePos[1] = m_curMousePos[1];

		m_curMousePos[0] = curPoint->x;
		m_curMousePos[1] = curPoint->y;

		m_deltaMPos[0] = m_curMousePos[0] - m_preMousePos[0];
		m_deltaMPos[1] = m_curMousePos[1] - m_preMousePos[1];
	}
}
