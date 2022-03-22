#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

//#include <cstdio>
//#include <stdlib.h>

#ifdef IMGUI

#include "imgui.h"

#ifdef DX11_RENDERER
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "RenderAPI/DX11/DX11Global.h"
#include "RenderAPI/DX11/Renderer.h"

#endif

#endif

class Scene;

#ifdef _WIN32

#include <Windows.h>

inline WNDPROC g_oldHWNDHandle = 0;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

inline LRESULT WndHandle2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	if (g_oldHWNDHandle) return g_oldHWNDHandle(hWnd, uMsg, wParam, lParam);

	return false;
}

void InitImgui(HWND hwnd);

inline void DestroyImgui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

namespace ImGuiCommon
{
	
class Console
{
private:
	constexpr inline static size_t INPUT_BUF_SIZE = 8 * 1024;
	constexpr inline static size_t BUF_SIZE = 8 * 1024;

	//fot std::cout
	std::streambuf* m_coutbuf = 0;
	std::streambuf* m_cerrbuf = 0;
	std::stringstream m_outBuf;
	std::stringstream m_errBuf;

	////for printf
	//char m_tempBuffer1[BUF_SIZE] = {};
	//char m_tempBuffer2[BUF_SIZE] = {};
	//decltype(stdout) m_oldStdOut = 0;
	//decltype(stderr) m_oldStdErr = 0;
	//decltype(stdout) m_newStdOut = 0;
	//decltype(stderr) m_newStdErr = 0;

	std::vector<std::string> m_outHistory;
	std::vector<std::string> m_errHistory;
	// text, error?
	std::vector<std::pair<int, int>> m_logHistoryView;
	//std::vector<std::string> m_commandHistory;

	
	std::string m_input;
	std::string m_resizedInput;

public:
	inline Console()
	{
		m_input.resize(INPUT_BUF_SIZE);
		m_resizedInput.reserve(INPUT_BUF_SIZE);

		//m_newStdOut = fmemopen(m_tempBuffer1, BUF_SIZE, "w");
		//m_newStdErr = fmemopen(m_tempBuffer1, BUF_SIZE, "w");
	};

	inline void RedirectStdOutput()
	{
		m_coutbuf = std::cout.rdbuf();
		m_cerrbuf = std::cerr.rdbuf();

		std::cout.rdbuf(m_outBuf.rdbuf());
		std::cerr.rdbuf(m_errBuf.rdbuf());
	};

	inline void RestoreStdOutput()
	{
		std::cout.rdbuf(m_coutbuf);
		std::cerr.rdbuf(m_cerrbuf);

		m_logHistoryView.push_back({ m_outHistory.size(), 0 });
		m_logHistoryView.push_back({ m_errHistory.size(), 1 });

		m_outHistory.push_back(m_outBuf.str());
		m_errHistory.push_back(m_errBuf.str());

		m_outBuf.str("");
		m_errBuf.str("");
	};

	inline void ClearOutput()
	{
		m_outHistory.clear();
		m_errHistory.clear();
		m_logHistoryView.clear();

		m_outBuf.str("");
		m_errBuf.str("");
	};

	/*inline void ClearCommand()
	{
		m_commandHistory.clear();
	};*/

	void Update(Scene* scene);

};

}

#endif