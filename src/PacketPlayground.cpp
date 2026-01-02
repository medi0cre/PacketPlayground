#include "PacketPlayground.h"

void PacketPlayground::Init() 
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup failed.\n";
        std::exit(1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
    {
        std::cerr << "Version 2.2 of Winsock not available.\n";
        WSACleanup();
        std::exit(2);
    }
}

void PacketPlayground::StartServer() 
{
    m_server.Start();
}

void PacketPlayground::StartClient()
{
    m_client.Start();
}