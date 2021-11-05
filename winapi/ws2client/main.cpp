/*
 * main.cpp
 * Winsock2 client example
 */

#define WIN32_LEAN_AND_MEAN

#include "ws2tools.h"
#include "toolbox.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

class Client
{
private:
    static void _receive(SOCKET sock, std::ostream &os);
    static SOCKET _connect(struct addrinfo *info);
    static SOCKET _connect(const char *host);
    static void _getAddrInfo(struct addrinfo **ret, const char *host);
    static int _sendString(SOCKET sock, const char *s);
public:
    static int run(int argc, char **argv);
    static int run(const char *host, std::ostream &os);
};

void Client::_getAddrInfo(struct addrinfo **ret, const char *host)
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult = ::getaddrinfo(host, DEFAULT_PORT, &hints, ret);

    if (iResult != 0)
        throw "getaddrinfo failed";
}

SOCKET Client::_connect(const char *host)
{
    struct addrinfo *result = NULL;
    _getAddrInfo(&result, host);
    SOCKET sock = 0;
    bool fError = true;

    try
    {
        sock = _connect(result);
        fError = false;
    }
    catch (...)
    {

    }

    freeaddrinfo(result);

    if (fError)
        throw "socket failed";

    return sock;
}

SOCKET Client::_connect(struct addrinfo *info)
{
    // Attempt to connect to an address until one succeeds
    for (struct addrinfo *ptr = info; ptr != NULL; ptr = ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        SOCKET sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (sock == INVALID_SOCKET)
            throw "socket failed";

        // Connect to server.
        if (::connect(sock, ptr->ai_addr, int(ptr->ai_addrlen)) == SOCKET_ERROR)
        {
            ::closesocket(sock);
            continue;
        }

        return sock;
    }

    throw "should never get here!";
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    std::string sCmdLine = Toolbox::wstrtostr(lpCmdLine);
    return Client::run(sCmdLine.c_str(), std::cout);
}
#else
int main(int argc, char **argv)
{
    return Client::run(argc, argv);
}
#endif

int Client::run(int argc, char **argv)
{
    // Validate the parameters
    if (argc != 2)
    {
        std::cerr << "usage: ws2client server-name\n";
        return 1;
    }

    return run(argv[1], std::cout);
}

/*!
 * \brief Client::_sendString
 * \param sock
 * \param s
 * \return
 *
 * Send NUL terminated string
 */
int Client::_sendString(SOCKET sock, const char *s)
{
    const int len = int(strlen(s));
    const int ret = ::send(sock, s, len, 0);

    if (ret == SOCKET_ERROR)
        throw "send string failed";

    return ret;
}

int Client::run(const char *host, std::ostream &os)
{
    try
    {
        WinSock2 ws2;
        SOCKET sock = _connect(host);

        try
        {
            const int sent = _sendString(sock, "this is a test");
            os << "Bytes Sent: " << sent << "\n";

            // shutdown the connection since no more data will be sent
            if (::shutdown(sock, SD_SEND) == SOCKET_ERROR)
                throw "shutdown failed";

            _receive(sock, os);
        }
        catch (...)
        {
            os << "Unknown error";
        }

        os.flush();
        ::closesocket(sock);
    }
    catch (const char *err)
    {
        os << err << "\n";
    }
    catch (...)
    {
        os << "Unknown error!\n";
    }

    os.flush();
    return 0;
}

void Client::_receive(SOCKET ConnectSocket, std::ostream &os)
{
    // Receive until the peer closes the connection
    while (true)
    {
        char recvbuf[DEFAULT_BUFLEN];
        const int ret = ::recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);

        if (ret == 0)
        {
            os << "Connection closed\n";
            break;
        }

        if (ret < 0)
            throw "recv failed";

        os << "Bytes received: " << ret << "\n";
    }
}

