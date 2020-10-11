#include "ws2tools.h"
#include "toolbox.h"
#include <iostream>
#include <fstream>
#include <cwchar>

#define DEFAULT_PORT "27015"

class Options
{
private:
    bool _fStdout;
    std::string _outputfn;
public:
    Options();
    void parse(int argc, char **argv);
    void parse(const wchar_t *cmdLine);
    bool fStdout() const;
    std::string outputFn() const;
};

class Server
{
private:
    static AddrInfo _getAddrInfo();
    static void _receive(Socket &sock, int buflen, std::ostream &os);
public:
    void run(std::ostream &os);
};

Options::Options() : _fStdout(false)
{

}

bool Options::fStdout() const
{
    return _fStdout;
}

std::string Options::outputFn() const
{
    return _outputfn;
}

void Options::parse(int argc, char **argv)
{
    if (argc == 1)
    {
        _fStdout = true;
        return;
    }

    _outputfn = std::string(argv[1]);
}

void Options::parse(const wchar_t *cmdLine)
{
    size_t len = ::wcslen(cmdLine);

    if (len < 2)
    {
        _fStdout = true;
        return;
    }

    _outputfn = Toolbox::wstrtostr(cmdLine);
}

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
    Options o;
    o.parse(lpCmdLine);
    Server server;

    if (o.fStdout())
    {
        server.run(std::cout);
        return 0;
    }

    std::ofstream ofs;
    ofs.open(o.outputFn().c_str(), std::ofstream::out | std::ofstream::binary);
    server.run(ofs);
    ofs.close();
    return 0;
}
#else
int main(int argc, char **argv)
{
    Options o;
    o.parse(argc, argv);
    Server server;

    if (o.fStdout())
    {
        server.run(std::cout);
        return 0;
    }

    std::ofstream ofs;
    ofs.open(o.outputFn().c_str(), std::ofstream::out | std::ofstream::binary);
    server.run(ofs);
    ofs.close();
    return 0;
}
#endif

