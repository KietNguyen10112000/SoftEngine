#pragma once

#ifdef _WIN32

class KEYBOARD
{
public:
	enum ENUM {
		MOUSE_LEFT = 250,
		MOUSE_MID = 251,
		MOUSE_RIGHT = 252,

		LSHIFT = 0xA0,
		CTRL = 0x11,
		TAB = 0x09,
		ESC = 0x1B,
		SPACE = 0x20,

		UP = 0x26,
		DOWN = 0x28,
		LEFT = 0x25,
		RIGHT = 0x27,
	};

};

#endif // WIN32
