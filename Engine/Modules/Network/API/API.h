#pragma once
#include "Core/TypeDef.h"

#include "Network/TypeDef.h"

#define NAMESPACE_SOCKET_API_BEGIN NAMESPACE_BEGIN namespace socketapi {
#define NAMESPACE_SOCKET_API_END } NAMESPACE_END

NAMESPACE_SOCKET_API_BEGIN

void Initialize();
void Finalize();

SOCKET_HANDLE CreateSocket(const TCP_SOCKET_DESCRIPTION& desc, byte* buffer, int sizeofBuffer);
void DestroySocket(SOCKET_HANDLE handle);
void SetBlockingMode(SOCKET_HANDLE handle, bool isBlockingMode);

int TranslateErrorCode(int input);

int Connect(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer);

bool IsReadyRead(SOCKET_HANDLE sock, long sec, long usec);
bool IsReadyWrite(SOCKET_HANDLE sock, long sec, long usec);

NAMESPACE_SOCKET_API_END