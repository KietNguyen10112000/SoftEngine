#pragma once
#include <Windows.h>
#include "Input.h"
#include <string>

class Window
{
private:
	HWND m_hwnd = NULL;

protected:
	int m_width = 0;
	int m_height = 0;

	std::wstring m_title;

	Input* m_input = nullptr;

public:
	//width, height resolution
	Window(const wchar_t* title, int& width, int& height, bool fullScreen = false);
	virtual ~Window();

public:
	//must be expand
	inline virtual void Update() {};

public:
	void Loop();

public:
	auto& GetNativeHandle() const { return m_hwnd; };

};