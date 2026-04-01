#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

namespace HTTP
{
    class Server
    {
        private:
		    int ConnectionCount = 0;
            pollfd* ConnectionList = nullptr;
            char* MessageBuffer = nullptr;

        public:
            Server(const char* IPAddress, const char* Port);
            ~Server();
	    	void Run();
    };
}
