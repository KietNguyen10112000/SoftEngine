#include "Graphics.h"

#include "Core/Memory/Memory.h"

#include "Detail/Detail.h"

#include "Platform/Platform.h"

#include "Network/TCPConnector.h"

NAMESPACE_BEGIN

int InvokeShaderCompiler(GRAPHICS_BACKEND_API backendAPI)
{
	/*std::string_view file_path = __FILE__;
	std::string_view dir_path = file_path.substr(0, file_path.rfind("\\"));
	std::string msg;

	auto exePath = platform::GetExecutablePath();
	std::string_view destPath = exePath.c_str();
	destPath = destPath.substr(0, destPath.rfind("\\"));

	switch (backendAPI)
	{
#ifdef _WIN32
	case soft::GRAPHICS_BACKEND_API::DX12:
		msg =
			".vs.hlsl->.vs.cso \"dxc/dxc.exe\" -T vs_6_0 -E main <input> -Fo <output>, "
			".ps.hlsl->.ps.cso \"dxc/dxc.exe\" -T ps_6_0 -E main <input> -Fo <output>, "
			"Src " + std::string(dir_path) + "/API/DX12/Shaders/, "
			"Dest " + std::string(destPath) + "/Shaders/";
		break;
#endif

	default:
		assert(0);
		break;
	}*/

#ifdef DEV
	TCP_SOCKET_DESCRIPTION desc = {};
	desc.host = "127.0.0.1";
	desc.port = 2222;

	TCPConnector tcp{ desc };
	auto ret = tcp.Connect();
	if (ret == SOCKET_ERCODE::CONNECT_TIMEOUT || ret == SOCKET_ERCODE::CONNECT_REFUSED)
	{
		std::cout << "[ERROR]: Graphics dev needs enable shader compiler server in directory 'DevTools/'\n";
		return -1;
	}

	const char* msg = "DX12";
	auto send = tcp.Send((byte*)msg, (int)4);

	char buf[1025] = {};
	int len = tcp.Recv((byte*)buf, 1024);
	buf[len] = 0;

	if (::strcmp(buf, "OK") != 0)
	{
		std::cout << "[ERROR]: error while compiling shaders\n" << buf;

		while (tcp.Recv((byte*)buf, 1024) != 0)
		{
			buf[len] = 0;
			std::cout << buf;
		}

		std::cout << "\n";

		return -1;
	}

#endif // DEV

	return 0;
}

int Graphics::Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI)
{
	auto _ret = InvokeShaderCompiler(backendAPI);
	if (_ret != 0)
	{
		return _ret;
	}

	if (s_instance.get() != nullptr)
	{
		assert(0);
	}

	Graphics* ret = nullptr;

	switch (backendAPI)
	{

#ifdef _WIN32
	case soft::GRAPHICS_BACKEND_API::DX12:
		ret = rheap::New<dx12::DX12Graphics>(windowNativeHandle);
		ret->m_debugGraphics = rheap::New<dx12::DX12DebugGraphics>((dx12::DX12Graphics*)ret);
		s_instance.reset(ret);
		break;
#endif

	default:
		assert(0);
		break;
	}

	return 0;
}

void Graphics::Finalize()
{
	auto graphics = s_instance.release();

	if (!graphics) return;

	rheap::Delete(graphics->m_debugGraphics);
	rheap::Delete(graphics);
}

NAMESPACE_END