#include "TCPConnector.h"

#include "API/API.h"

#ifdef _WIN32
#include <WinSock2.h>
#endif // _WIN32


NAMESPACE_BEGIN

#define FORWARD_OPAQUE(o)  o, sizeof(o) / sizeof(o[0])

TCPConnector::TCPConnector(const TCP_SOCKET_DESCRIPTION& desc)
{
	m_sock = socketapi::CreateSocket(desc, FORWARD_OPAQUE(m_remoteOpaque));
}

TCPConnector::~TCPConnector()
{
	socketapi::DestroySocket(m_sock);
}

int TCPConnector::Connect()
{
	socketapi::SetBlockingMode(m_sock, false);

	auto ret = socketapi::Connect(m_sock, FORWARD_OPAQUE(m_localOpaque), FORWARD_OPAQUE(m_remoteOpaque));

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

int TCPConnector::Disconnect()
{
	socketapi::DestroySocket(m_sock);
	m_sock = nullptr;
	memset(m_localOpaque, 0, sizeof(m_localOpaque));
	memset(m_remoteOpaque, 0, sizeof(m_remoteOpaque));
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

void TCPConnector::SetBlockingMode(bool isBlockingNode)
{
	socketapi::SetBlockingMode(m_sock, isBlockingNode);
}

bool TCPConnector::ReadyForRecv()
{
	return socketapi::IsReadyRead(m_sock, 0, 1);
}

String TCPConnector::GetAddressString()
{
	char addrStr[256] = {};
	socketapi::ConvertAddrToStr(FORWARD_OPAQUE(m_localOpaque), addrStr);
	return addrStr;
}

String TCPConnector::GetPeerAddressString()
{
	char addrStr[256] = {};
	socketapi::ConvertAddrToStr(FORWARD_OPAQUE(m_remoteOpaque), addrStr);
	return addrStr;
}

NAMESPACE_END