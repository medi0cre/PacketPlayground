#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace HTTP
{
    struct Request
    {
        std::string Method = "";
        std::string URI = "";
        std::string Version = "";
        std::unordered_map<std::string, std::string> Headers{};
        std::string Body = "";
    };
	
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
		bool BlankLineFound = false;
        pollfd Socket{};
        std::string MessageBuffer = "";
		Request ClientRequest{};
		Response ClientResponse{};
    };

    class ResponseBuilder
    {
	private:
	    Response Res{};

	public:
	    ResponseBuilder() = default;
	    ~ResponseBuilder() = default;
		
        Response Build();
	    ResponseBuilder& Reset();
        ResponseBuilder& Version(const std::string& Version); 
	    ResponseBuilder& StatusCode(int StatusCode);
	    ResponseBuilder& Status(const std::string& Status);
	    ResponseBuilder& Body(const std::string& Body);
	    ResponseBuilder& Header(const std::string& Key, const std::string& Value);
		
        ResponseBuilder& NotFound();
		ResponseBuilder& BadRequest();

		ResponseBuilder& OK();
		ResponseBuilder& JSON();
		ResponseBuilder& HTML();
    };

    class Server
    {
    private:
        static constexpr int MaxConnections = 32;
        static constexpr int BufferSize = 32768;

        std::vector<Connection> ConnectionList{};
	    std::unordered_map<std::string, std::function<Response(const Request&)>> Routes{};

	    void HandleNewConnection();
	    void HandleClientData(int Index);
	    void SendResponse(int ClientFD, const Response& Res);
        std::string CPPString(const Response& Res);

    public:
        Server(const char* IPAddress, const char* Port);
        ~Server();
	    void Run();
	    void AddRoute(const std::string& Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher);
    };
}
