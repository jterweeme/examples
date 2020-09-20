#include "ws2tools.h"
#include <iostream>

#define DEFAULT_PORT "27015"

class Server
{
private:
    static AddrInfo _getAddrInfo();
    static void _receive(Socket &sock, int buflen, std::ostream &os);
public:
    void run(std::ostream &os);
};

AddrInfo Server::_getAddrInfo()
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    return AddrInfo(DEFAULT_PORT, &hints);
}

void Server::_receive(Socket &sock, int buflen, std::ostream &os)
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
                sock.shutdown(SD_SEND);
                break;
            }

            os.write(buf, ret);
            os.flush();
        }
    }
    catch (...)
    {
        os << "error\n";
        os.flush();
    }

    delete[] buf;
}

void Server::run(std::ostream &os)
{
    try
    {
        WinSock2 ws2;
        ws2.init();
        AddrInfo info = _getAddrInfo();
        Socket listenSock(&info);
        listenSock.init();
        listenSock.bind();
        listenSock.listen();
        Socket clientSock = listenSock.accept();
        listenSock.close();
        _receive(clientSock, 512, os);
    }
    catch (...)
    {

    }
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    Server server;
    server.run(std::cout);
    return 0;
}
#else
int main()
{
    Server server;
    server.run(std::cout);
    return 0;
}
#endif

