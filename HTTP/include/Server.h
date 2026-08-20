#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Parser.h>

constexpr size_t MaxConnections = 32;
constexpr size_t MaxBufferSize = 32768;

namespace HTTP
{
    struct Response
    {
        uint16_t StatusCode = 0;
        std::string Status = "";
        std::string Version = "";
        std::vector<KV> Headers{};
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
        void HandleClientData(size_t Index);
        void SendResponse(SOCKET ClientFD, const Response& Res);
        void RemoveConnection(size_t Index);
        void ProcessRequest(size_t Index, const Request& Req);

    public:
        Server(const char* IPAddress, const char* Port);
        ~Server();
        void Run();
        void AddRoute(std::string Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher);
    };
}
