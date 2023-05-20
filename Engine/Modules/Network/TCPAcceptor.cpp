#include "TCPAcceptor.h"

#include "TCPConnector.h"

#include "API/API.h"

#define FORWARD_OPAQUE(o)  o, sizeof(o) / sizeof(o[0])

NAMESPACE_BEGIN

TCPAcceptor::TCPAcceptor(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient)
{
	Initialize(desc, maxClient);
}

TCPAcceptor::~TCPAcceptor()
{
	socketapi::DestroySocket(m_sock);
}

int TCPAcceptor::Initialize(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient)
{
	m_sock = socketapi::CreateSocket(desc, FORWARD_OPAQUE(m_opaque));
	m_maxClient = maxClient;

	socketapi::Bind(m_sock, FORWARD_OPAQUE(m_opaque));
	socketapi::Listen(m_sock, FORWARD_OPAQUE(m_opaque), (int)m_maxClient);
	return 0;
}

int TCPAcceptor::Accept(TCPConnector& output)
{
	auto ret = socketapi::Accept(m_sock, FORWARD_OPAQUE(m_opaque),
		(int)m_maxClient, output.m_sock, 
		FORWARD_OPAQUE(output.m_localOpaque), FORWARD_OPAQUE(output.m_remoteOpaque));

	return ret;
}

NAMESPACE_END