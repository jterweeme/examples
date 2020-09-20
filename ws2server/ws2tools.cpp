#include "ws2tools.h"

AddrInfo::AddrInfo(const char *svrName, struct addrinfo *hints)
{
    if (::getaddrinfo(NULL, svrName, hints, &_info) != 0)
        throw "getaddrinfo failed";
}

AddrInfo::~AddrInfo()
{
    ::freeaddrinfo(_info);
}

int AddrInfo::socktype() const
{
    return _info->ai_socktype;
}

int AddrInfo::protocol() const
{
    return _info->ai_protocol;
}

int AddrInfo::family() const
{
    return _info->ai_family;
}

sockaddr *AddrInfo::addr() const
{
    return _info->ai_addr;
}

size_t AddrInfo::addrLen() const
{
    return _info->ai_addrlen;
}

void WS2Tools::initWinsock(WSAData *wsaData)
{
    if (WSAStartup(MAKEWORD(2, 2), wsaData) != 0)
        throw "WSAStartup error";
}

SOCKET WS2Tools::socket(const AddrInfo &info)
{
    SOCKET ret = ::socket(info.family(), info.socktype(), info.protocol());

    if (ret == INVALID_SOCKET)
        throw "socket failed";

    return ret;
}

SOCKET WS2Tools::socket(struct addrinfo *info)
{
    SOCKET ret = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);

    if (ret == INVALID_SOCKET)
        throw "socket failed";

    return ret;
}

void WS2Tools::bind(SOCKET sock, const AddrInfo &info)
{
    if (::bind(sock, info.addr(), int(info.addrLen())) == SOCKET_ERROR)
        throw "bind failed";
}

void WS2Tools::bind(SOCKET sock, struct addrinfo *result)
{
    if (::bind(sock, result->ai_addr, int(result->ai_addrlen)) == SOCKET_ERROR)
        throw "bind failed";
}

WinSock2::WinSock2() : _fInitialized(false)
{

}

WinSock2::~WinSock2()
{
    if (_fInitialized)
        WSACleanup();
}

void WinSock2::init()
{
    WS2Tools::initWinsock(&_wsaData);
    _fInitialized = true;
}

Socket::Socket(const AddrInfo *info) : _info(info), _fFlag(false)
{

}

Socket::Socket(const SOCKET &sock) : _sock(sock)
{
}

Socket::~Socket()
{
    if (_fFlag)
        ::closesocket(_sock);
}

void Socket::close()
{
    ::closesocket(_sock);
    _fFlag = false;
}

void Socket::bind()
{
    WS2Tools::bind(_sock, *_info);
}

void Socket::init()
{
    _sock = WS2Tools::socket(*_info);
    _fFlag = true;
}

void Socket::listen()
{
    if (::listen(_sock, SOMAXCONN) == SOCKET_ERROR)
        throw "cannot listen!";
}

Socket Socket::accept()
{
    SOCKET ret = ::accept(_sock, NULL, NULL);
    return Socket(ret);
}

int Socket::send(const char *buf, int len)
{
    int ret = ::send(sock(), buf, len, 0);

    if (ret == SOCKET_ERROR)
        throw "send failed";

    return ret;
}

int Socket::recv(char *buf, int len)
{
    int ret = ::recv(_sock, buf, len, 0);

    if (ret < 0)
        throw "recv failed";

    return ret;
}

void Socket::shutdown(int how)
{
    if (::shutdown(_sock, how) == SOCKET_ERROR)
        throw "shutdown failed";
}

SOCKET Socket::sock() const
{
    return _sock;
}

