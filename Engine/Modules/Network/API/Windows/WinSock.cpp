#include "../API.h"

#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

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
        return 0;
    case WSAECONNREFUSED:
        return SOCKET_ERCODE::CONNECT_REFUSED;
    default:
        Throw("");
        break;
    }
}

int Connect(SOCKET_HANDLE handle, byte* buffer, int sizeofBuffer)
{
    int& addrLen = *(int*)&buffer[sizeofBuffer - 4];
    if (connect((SOCKET)handle, (SOCKADDR*)buffer, addrLen) == SOCKET_ERROR)
    {
        return TranslateErrorCode(SOCKET_ERROR);
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

NAMESPACE_SOCKET_API_END