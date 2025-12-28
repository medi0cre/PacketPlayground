#include "Server.h"

void Server::Start() 
{
    m_socket.ResetMemory();
    m_socket.SetFamily(AF_INET);
    m_socket.SetSocketType(SOCK_STREAM);
    m_socket.SetFlag(AI_PASSIVE);

    m_socket.GetAddressInfo(NULL, "3490");

    for (addrinfo* p = m_socket.Info(); p != NULL; p = p->ai_next) 
    {
        m_socket.Init();

        const char options = 1;

        if (setsockopt(m_socket.FileDesc(), SOL_SOCKET, SO_REUSEADDR, &options, sizeof(int)) == -1)
        {
            std::cerr << "Error setting socket options\n";
            std::exit(1);
        }

        if (p == NULL) 
        {
            std::cerr << "Server: Failed to bind port\n";
            std::exit(1);
        }

        break;
    }

    m_socket.FreeAddress();
    m_socket.SetConnectionCount(10);
    m_socket.Listen();

    std::cout << "Waiting for connections...\n";

    while (true) 
    {
        m_socket.Accept();
        
        void* input = nullptr;

        sockaddr_storage& storage = m_socket.Storage();
        sockaddr* address = reinterpret_cast<sockaddr*>(&storage);

        if (address->sa_family == AF_INET) 
        {
            input = &(reinterpret_cast<sockaddr_in*>(address)->sin_addr);
        }
        else 
        {
            input = &(reinterpret_cast<sockaddr_in6*>(address)->sin6_addr);
        }

        inet_ntop(storage.ss_family, input, m_socket.IP, sizeof m_socket.IP);

        std::cout << "Server: Obtained connection from " << m_socket.IP << "\n";
    }
}