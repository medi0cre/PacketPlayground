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
        uint16_t StatusCode = 200;
        std::string Status = "OK";
        std::unordered_map<std::string, std::string> Headers{};
        std::string Body = "";
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
			void SendResponse(uint8_t ClientFD, const Response& Res);
			void RemoveConnection(uint8_t Index);
			Request ParseRequest(const std::string& RawData);

        public:
            Server(const char* IPAddress, const char* Port);
            ~Server();
	    	void Run();
			void AddRoute(const std::string& Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher);
    };
}
