#pragma once
#include "Core/TypeDef.h"

#include "Network/TypeDef.h"

#define NAMESPACE_SOCKET_API_BEGIN NAMESPACE_BEGIN namespace socketapi {
#define NAMESPACE_SOCKET_API_END } NAMESPACE_END

NAMESPACE_SOCKET_API_BEGIN

void Initialize();
void Finalize();

SOCKET_HANDLE CreateSocket(const TCP_SOCKET_DESCRIPTION& desc, 
    byte* buffer, int sizeofBuffer);

void DestroySocket(SOCKET_HANDLE handle);
void SetBlockingMode(SOCKET_HANDLE handle, bool isBlockingMode);

int TranslateErrorCode(int input);

int Connect(SOCKET_HANDLE handle, 
    byte* localOpaque, int sizeofLocalOpaque, 
    byte* remoteOpaque, int sizeofRemoteOpaque
);

int Bind(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer);
int Listen(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer, int maxClient);
int Accept(SOCKET_HANDLE handle, 
    byte* buffer, int sizeofBuffer, int maxClient,
    SOCKET_HANDLE& output, 
    byte* localOpaque, int sizeofLocalOpaque,
    byte* remoteOpaque, int sizeofRemoteOpaque
);

bool IsReadyRead(SOCKET_HANDLE sock, long sec, long usec);
bool IsReadyWrite(SOCKET_HANDLE sock, long sec, long usec);

void ConvertAddrToStr(byte* buffer, int sizeofBuffer, char* outputBuffer);

NAMESPACE_SOCKET_API_END