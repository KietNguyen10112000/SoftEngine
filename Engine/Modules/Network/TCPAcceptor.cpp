#include "TCPAcceptor.h"

#include "TCPConnector.h"

#include "API/API.h"

NAMESPACE_BEGIN

TCPAcceptor::TCPAcceptor(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient)
{
	m_sock = socketapi::CreateSocket(desc, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]));
	m_maxClient = maxClient;

	socketapi::Bind(m_sock, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]));
	socketapi::Listen(m_sock, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]), (int)m_maxClient);
}

TCPAcceptor::~TCPAcceptor()
{
	socketapi::DestroySocket(m_sock);
}

int TCPAcceptor::Accept(TCPConnector& output)
{
	socketapi::Accept(m_sock, m_opaque, sizeof(m_opaque) / sizeof(m_opaque[0]),
		(int)m_maxClient, output.m_sock, output.m_opaque, sizeof(output.m_opaque) / sizeof(output.m_opaque[0]));
	return 0;
}

NAMESPACE_END