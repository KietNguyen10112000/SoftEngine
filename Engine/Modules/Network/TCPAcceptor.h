#pragma once
#include "TypeDef.h"

NAMESPACE_BEGIN

class TCPConnector;

class API TCPAcceptor
{
private:
	SOCKET_HANDLE m_sock = nullptr;
	byte m_opaque[32] = {};
	size_t m_maxClient = 0;

public:
	inline TCPAcceptor() {};
	TCPAcceptor(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient);
	~TCPAcceptor();

public:
	int Initialize(const TCP_SOCKET_DESCRIPTION& desc, size_t maxClient);
	int Accept(TCPConnector& output);

};

NAMESPACE_END