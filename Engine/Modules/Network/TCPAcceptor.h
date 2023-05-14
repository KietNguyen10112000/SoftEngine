#pragma once
#include "TypeDef.h"

NAMESPACE_BEGIN

class TCPConnector;

class API TCPAcceptor
{
private:
	SOCKET_HANDLE m_sock = nullptr;
	byte m_opaque[32] = {};
	size_t m_maxClient;

public:
	TCPAcceptor(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient);
	~TCPAcceptor();

public:
	int Accept(TCPConnector& output);

};

NAMESPACE_END