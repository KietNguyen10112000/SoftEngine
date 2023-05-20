#include "../API.h"

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#include <stdio.h>
#include <iostream>

NAMESPACE_SOCKET_API_BEGIN 

static int IPVersion(const char* src) 
{
    char buf[16];
    if (inet_pton(AF_INET, src, buf)) 
    {
        return 4;
    }
    else if (inet_pton(AF_INET6, src, buf)) 
    {
        return 6;
    }
    return -1;
}

void Initialize()
{
    auto wsadata = WSADATA();
    // Initialize Winsock
    auto iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (iResult != 0)
    {
        Throw("socketapi::Initialize() failed.");
    }
}

void Finalize()
{
    WSACleanup();
}

SOCKET_HANDLE CreateSocket(const TCP_SOCKET_DESCRIPTION& desc, byte* buffer, int sizeofBuffer)
{
    int IPv = IPVersion(desc.host);
    if (IPv == -1)
    {
        Throw("CreateSocket() failed, invalid host.");
    }

    if (sizeofBuffer < sizeof(sockaddr_in6) + sizeof(int))
    {
        Throw("Error");
    }

    int af = IPv == 4 ? AF_INET : AF_INET6;

    auto sock = socket(af, SOCK_STREAM, IPPROTO_TCP);

    int& addrLen = *(int*)&buffer[sizeofBuffer - 4];

    if (af == AF_INET)
    {
        sockaddr_in* p = (sockaddr_in*)buffer;

        p->sin_family = af;
        p->sin_port = htons(desc.port);

        InetPtonA(af, desc.host, &p->sin_addr.s_addr);
        addrLen = sizeof(sockaddr_in);
    }
    else
    {
        sockaddr_in6* p = (sockaddr_in6*)buffer;

        p->sin6_family = af;
        p->sin6_port = htons(desc.port);

        InetPtonA(af, desc.host, &p->sin6_addr.s6_addr);
        addrLen = sizeof(sockaddr_in6);
    }

    if (desc.sendBufferSize)
    {
        int sendBuffer = desc.sendBufferSize;
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuffer, sizeof(int));
    }

    if (desc.recvBufferSize)
    {
        int recvBuffer = desc.recvBufferSize;
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recvBuffer, sizeof(int));
    }

    if (desc.useNonBlocking)
    {
        SetBlockingMode((SOCKET_HANDLE)sock, false);
    }

    return (SOCKET_HANDLE)sock;
}

void DestroySocket(SOCKET_HANDLE handle)
{
    /*auto ret = shutdown((SOCKET)handle, SD_SEND);
    if (ret != 0 && ret != WSAENOTCONN)
    {
        Throw("DestroySocket() failed.");
    }

    char recvbuf[128];

    int iResult = 0;
    do {
        iResult = recv((SOCKET)handle, recvbuf, 128, 0);
    } while (iResult > 0);*/

    closesocket((SOCKET)handle);
}

void SetBlockingMode(SOCKET_HANDLE handle, bool isBlockingMode)
{
    u_long mode = !isBlockingMode;  // 1 to enable non-blocking socket
    ioctlsocket((SOCKET)handle, FIONBIO, &mode);
}

int TranslateErrorCode(int input)
{
    input = WSAGetLastError();
    switch (input)
    {
    case WSAEWOULDBLOCK:
        return SOCKET_ERCODE::WOULD_BLOCK;
    case WSAEMSGSIZE:
        return SOCKET_ERCODE::MSG_SIZE;
    case WSAEALREADY:
        return SOCKET_ERCODE::WOULD_BLOCK;
    case WSAECONNREFUSED:
        return SOCKET_ERCODE::CONNECT_REFUSED;
    default:
        //Throw("");
        //break;
        return SOCKET_ERCODE::UNKNOWN;
    }
}

int Connect(SOCKET_HANDLE handle, 
    byte* localOpaque, int sizeofLocalOpaque,
    byte* remoteOpaque, int sizeofRemoteOpaque)
{
    int& remoteAddrLen = *(int*)&remoteOpaque[sizeofRemoteOpaque - 4];
    int& addrLen = *(int*)&localOpaque[sizeofLocalOpaque - 4];
    addrLen = remoteAddrLen;

    auto connectRet = connect((SOCKET)handle, (SOCKADDR*)remoteOpaque, remoteAddrLen);
    if (connectRet == SOCKET_ERROR)
    {
        auto ercode = TranslateErrorCode(WSAGetLastError());
        if (ercode != SOCKET_ERCODE::WOULD_BLOCK)
        {
            return ercode;
        }
    }

    int& localAddrLen = *(int*)&localOpaque[sizeofLocalOpaque - 4];
    localAddrLen = sizeofLocalOpaque - 4;
    getsockname((SOCKET)handle, (SOCKADDR*)localOpaque, &localAddrLen);

    return 0;
}

int Bind(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer)
{
    int ercode;
    int& addrLen = *(int*)&buffer[sizeofBuffer - 4];
    if ((ercode = bind((SOCKET)handle, (SOCKADDR*)buffer, addrLen)) != 0)
    {
        return TranslateErrorCode(ercode);
    }

    return 0;
}

int Listen(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer, int maxClient)
{
    int ercode;
    if ((ercode = listen((SOCKET)handle, maxClient)) != 0)
    {
        return TranslateErrorCode(ercode);
    }

    return 0;
}

int Accept(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer, int maxClient, 
    SOCKET_HANDLE& output, 
    byte* localOpaque, int sizeofLocalOpaque,
    byte* remoteOpaque, int sizeofRemoteOpaque)
{
    int ercode;
    int& addrLen = *(int*)&remoteOpaque[sizeofRemoteOpaque - 4];
    addrLen = sizeofRemoteOpaque - 4;
    output = (SOCKET_HANDLE)accept((SOCKET)handle, (SOCKADDR*)remoteOpaque, &addrLen);

    if ((size_t)output == INVALID_SOCKET)
    {
        ercode = WSAGetLastError();
        return TranslateErrorCode(ercode);
    }

    int& localAddrLen = *(int*)&localOpaque[sizeofLocalOpaque - 4];
    localAddrLen = sizeofLocalOpaque - 4;
    while (getsockname((SOCKET)output, (SOCKADDR*)&localOpaque, &localAddrLen) != 0)
    {
        std::cout << "Wait for get socket info...\n";
        Sleep(16);
    }

    return 0;
}

bool IsReadyRead(SOCKET_HANDLE sock, long sec, long usec)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET((SOCKET)sock, &fds);

    timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return (select(0, &fds, 0, 0, &tv) == 1);
}

bool IsReadyWrite(SOCKET_HANDLE sock, long sec, long usec)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET((SOCKET)sock, &fds);

    timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return (select(0, 0, &fds, 0, &tv) == 1);
}

void ConvertAddrToStr(byte* buffer, int sizeofBuffer, char* outputBuffer)
{
    sockaddr* sa = (sockaddr*)buffer;
    int& addrLen = *(int*)&buffer[sizeofBuffer - 4];

    if (sa->sa_family == AF_INET)
    {
        sockaddr_in* p = (sockaddr_in*)buffer;
        auto ret = inet_ntop(sa->sa_family, &p->sin_addr, outputBuffer, 256);

        if (ret == NULL)
        {
            std::cout << "ConvertAddrToStr error code: " << WSAGetLastError() << "\n";
        }

        auto len = strlen(outputBuffer);
        outputBuffer[len++] = ':';
        sprintf(outputBuffer + len, "%d", p->sin_port);
    }
    else
    {
        sockaddr_in6* p = (sockaddr_in6*)buffer;
        inet_ntop(sa->sa_family, &p->sin6_addr, outputBuffer, 256);
        auto len = strlen(outputBuffer);
        outputBuffer[len++] = ':';
        sprintf(outputBuffer + len, "%d", p->sin6_port);
    }
}

NAMESPACE_SOCKET_API_END