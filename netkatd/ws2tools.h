#ifndef WS2TOOLS_H
#define WS2TOOLS_H

#include <winsock2.h>
#include <ws2tcpip.h>

class AddrInfo
{
private:
    struct addrinfo *_info;
public:
    AddrInfo(const char *svrName, struct addrinfo *hints);
    ~AddrInfo();
    int family() const;
    sockaddr *addr() const;
    size_t addrLen() const;
    int socktype() const;
    int protocol() const;
};

class WS2Tools
{
public:
    static void initWinsock(WSADATA *wsaData);
    static SOCKET socket(struct addrinfo *info);
    static SOCKET socket(const AddrInfo &info);
    static void bind(SOCKET sock, struct addrinfo *result);
    static void bind(SOCKET sock, const AddrInfo &result);
};

class WinSock2
{
private:
    WSADATA _wsaData;
    bool _fInitialized;
public:
    WinSock2();
    ~WinSock2();
    void init();
};

class Socket
{
private:
    const AddrInfo *_info;
    SOCKET _sock;
    bool _fFlag;
    Socket(const SOCKET &sock);
public:
    Socket(const AddrInfo *info);
    ~Socket();
    void init();
    void bind();
    void listen();
    void close();
    int send(const char *buf, int len);
    int recv(char *buf, int len);
    void shutdown(int how);
    Socket accept();
    SOCKET sock() const;
};

#endif

