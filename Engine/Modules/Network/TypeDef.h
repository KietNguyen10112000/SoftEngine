#pragma once
#include "Core/TypeDef.h"

NAMESPACE_BEGIN

using SOCKET_HANDLE = void*;

struct SOCKET_DESCRIPTION
{
	// host, support ipv4 & ipv6
	const char*		host;
	uint16_t		port;

	uint32_t		recvBufferSize;
	uint32_t		sendBufferSize;

	bool			useSSL;
	bool			useNonBlocking;
};

using TCP_SOCKET_DESCRIPTION = SOCKET_DESCRIPTION;

struct SOCKET_ERCODE
{
	constexpr static int WOULD_BLOCK = -1;
	constexpr static int MSG_SIZE	= -2;

	constexpr static int CONNECT_TIMEOUT = -9998;
	constexpr static int CONNECT_REFUSED = -9999;

	constexpr static int UNKNOWN = -999999;
};

NAMESPACE_END