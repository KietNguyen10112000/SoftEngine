#pragma once
#include "TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

#include "Core/Structures/String.h"

NAMESPACE_BEGIN

class API TCPConnector
{
private:
	friend class TCPAcceptor;

	SOCKET_HANDLE m_sock = nullptr;

	byte m_localOpaque[32] = {};
	byte m_remoteOpaque[32] = {};

public:
	inline TCPConnector() {};
	TCPConnector(const TCP_SOCKET_DESCRIPTION& desc);
	~TCPConnector();

public:
	int Connect();
	int Disconnect();

	int Recv(std::Vector<byte>& buffer);
	int Recv(byte* buffer, int bufferSize);

	int Send(std::Vector<byte>& buffer);
	int Send(byte* buffer, int bufferSize);

	void SetBlockingMode(bool isBlockingNode);
	bool ReadyForRecv();

	// return the local address used to connect to peer address
	String GetAddressString();

	// return peer address (another one connector)
	String GetPeerAddressString();

	inline bool IsDisconnected() const
	{
		return m_sock == nullptr;
	}
};

NAMESPACE_END