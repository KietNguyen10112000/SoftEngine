#pragma once
#include "TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class TCPConnector
{
private:
	SOCKET_HANDLE m_sock = nullptr;
	byte m_opaque[32] = {};

public:
	TCPConnector(const TCP_SOCKET_DESCRIPTION& desc);
	~TCPConnector();

public:
	int Connect();

	int Recv(std::Vector<byte>& buffer);
	int Recv(byte* buffer, int bufferSize);

	int Send(std::Vector<byte>& buffer);
	int Send(byte* buffer, int bufferSize);

};

NAMESPACE_END