#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
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
        uint16_t StatusCode = 0;
        std::string Status = "";
        std::string Version = "";
        std::unordered_map<std::string, std::string> Headers{};
        std::string Body = "";
    };
	
    class ResponseBuilder
    {
	private:
	    Response Res{};

	public:
	    ResponseBuilder() = default;
	    ~ResponseBuilder() = default;
		
        Response& Build();
	    ResponseBuilder& Reset();
        ResponseBuilder& Version(const std::string& Version); 
	    ResponseBuilder& StatusCode(uint16_t StatusCode);
	    ResponseBuilder& Status(const std::string& Status);
	    ResponseBuilder& Body(const std::string& Body);
	    ResponseBuilder& Header(const std::string& Key, const std::string& Value);
    };

    class Server
    {
    private:
	    static constexpr uint8_t MaxConnections = 32;
	    static constexpr uint16_t BufferSize = 32768;
	    static constexpr uint16_t ReadBufferSize = 8192;

	    uint8_t ConnectionCount = 0;
        pollfd* ConnectionList = nullptr;
        char* MessageBuffer = nullptr;
	    std::unordered_map<std::string, std::function<Response(const Request&)>> Routes{};
			
	    void HandleNewConnection();
	    void HandleClientData(uint8_t Index);
	    void SendResponse(int32_t ClientFD, const Response& Res);
	    void RemoveConnection(uint8_t Index);
        
        std::string CreateResponseString(const Response& Res);
	    Request ParseRequest(const std::string& RawData);

    public:
        Server(const char* IPAddress, const char* Port);
        ~Server();
	    void Run();
	    void AddRoute(const std::string& Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher);
    };
}
