#include "TCPConnector.h"

#include "API/API.h"

#ifdef _WIN32
#include <WinSock2.h>
#endif // _WIN32


NAMESPACE_BEGIN

TCPConnector::TCPConnector(const TCP_SOCKET_DESCRIPTION& desc)
{
	m_sock = socketapi::CreateSocket(desc, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]));
}

TCPConnector::~TCPConnector()
{
	socketapi::DestroySocket(m_sock);
}

int TCPConnector::Connect()
{
	socketapi::SetBlockingMode(m_sock, false);

	auto ret = socketapi::Connect(m_sock, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]));

	if (ret < 0 && ret != SOCKET_ERCODE::WOULD_BLOCK)
	{
		return ret;
	}

	if (!socketapi::IsReadyWrite(m_sock, 5, 0))
	{
		socketapi::SetBlockingMode(m_sock, true);
		return SOCKET_ERCODE::CONNECT_TIMEOUT;
	}

	socketapi::SetBlockingMode(m_sock, true);
	return 0;
}

int TCPConnector::Recv(std::Vector<byte>& buffer)
{
	auto ret = recv((SOCKET)m_sock, (char*)&buffer[0], buffer.size(), 0);
	if (ret < 0)
	{
		return socketapi::TranslateErrorCode(ret);
	}
	return ret;
}

int TCPConnector::Recv(byte* buffer, int bufferSize)
{
	auto ret = recv((SOCKET)m_sock, (char*)buffer, bufferSize, 0);
	if (ret < 0)
	{
		return socketapi::TranslateErrorCode(ret);
	}
	return ret;
}

int TCPConnector::Send(std::Vector<byte>& buffer)
{
	auto ret = send((SOCKET)m_sock, (char*)&buffer[0], buffer.size(), 0);
	if (ret < 0)
	{
		return socketapi::TranslateErrorCode(ret);
	}
	return ret;
}

int TCPConnector::Send(byte* buffer, int bufferSize)
{
	auto ret = send((SOCKET)m_sock, (char*)buffer, bufferSize, 0);
	if (ret < 0)
	{
		return socketapi::TranslateErrorCode(ret);
	}
	return ret;
}

NAMESPACE_END