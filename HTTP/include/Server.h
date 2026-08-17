#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Parser.h>

#define MaxConnections 32
#define BufferSize 32768

namespace HTTP
{
    struct Response
    {
        int StatusCode = 0;
        std::string Status = "";
        std::string Version = "";
        std::unordered_map<std::string, std::string> Headers{};
        std::string Body = "";
    };

    struct Connection
    {
        WSAPOLLFD Socket{};
        Parser Par{};
    };

    class Server
    {
    private:
        std::vector<Connection> ConnectionList{};
        std::unordered_map<std::string, std::function<Response(const Request&)>> Routes{};

        std::string Stringify(const Response& Res);
        void HandleNewConnection();
        void HandleClientData(int Index);
        void SendResponse(SOCKET ClientFD, const Response& Res);
        void RemoveConnection(int Index);
        void ProcessRequest(int Index, const Request& Req);

    public:
        Server(const char* IPAddress, const char* Port);
        ~Server();
        void Run();
        void AddRoute(std::string Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher);
    };
}
