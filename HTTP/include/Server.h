#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Parser.h>

constexpr size_t MaxConnections = 32;
constexpr size_t MaxBufferSize = 32768;
constexpr size_t ReceiveBufferSize = 8192;
constexpr size_t MaxTimeout = 5000; // milliseconds

using Clock = std::chrono::steady_clock;
using Time = Clock::time_point;
using MS = std::chrono::milliseconds;

namespace PPG
{
    struct StringKV
    {
        std::string Key = "";
        std::string Value = "";
    };

    struct Response
    {
        uint16_t StatusCode = 0;
        std::string Status = "";
        std::string Version = "";
        std::vector<StringKV> Headers{};
        std::string Body = "";
    };

    struct Connection
    {
        WSAPOLLFD Socket{};
        Parser Par{};
        Time LastEvent{};
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
        void AddRoute(std::string_view Method, std::string_view Path, std::function<Response(const Request&)> Dispatcher);
    };
}
