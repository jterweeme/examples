/*
 * main.cpp
 * Winsock2 server example
 */

#include "ws2tools.h"
#include <windows.h>
#include <iostream>

class Server
{
private:
    static AddrInfo _getAddrInfo();
    static void _receive(Socket &sock, std::ostream &os, const int buflen);
public:
    int run(std::ostream &os);
};

AddrInfo Server::_getAddrInfo()
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    return AddrInfo("27015", &hints);
}

int Server::run(std::ostream &os)
{
    try
    {
        os << "Starting ws2server example...\r\n";
        os.flush();
        WinSock2 ws2;
        ws2.init();
        AddrInfo info = _getAddrInfo();
        Socket listenSock(&info);
        listenSock.init();
        listenSock.bind();
        listenSock.listen();
        Socket clientSock = listenSock.accept();
        listenSock.close();
        _receive(clientSock, os, 512);
    }
    catch (...)
    {
        os << "Unknown error\r\n";
        os.flush();
    }

    return 0;
}

void Server::_receive(Socket &sock, std::ostream &os, const int buflen)
{
    char *buf = new char[buflen];

    try
    {
        //Receive until the peer shuts down the connection
        while (true)
        {
            int ret = sock.recv(buf, buflen);

            if (ret == 0)
            {
                os << "Connection closing\n";
                os.flush();
                sock.shutdown(SD_SEND);
                break;
            }

            os << "Bytes received: " << ret << "\n";
            os.flush();

            // Echo the buffer back to the sender
            int iSendResult = sock.send(buf, ret);
            os << "Bytes sent: " << iSendResult << "\n";
            os.flush();
        }
    }
    catch (...)
    {
    }

    delete[] buf;
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    Server server;
    return server.run(std::cout);
}
#else
int main()
{
    Server server;
    return server.run(std::cout);
}
#endif
