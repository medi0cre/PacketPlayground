#include <cstring>
#include <iostream>

#include "Utils.h"
#include "Server.h"

HTTP::Server::Server(const char* IPAddress, const char* Port)
{
	// Load winsock API version 2.2
    WSADATA Data{};
    Enforce(WSAStartup(MAKEWORD(2, 2), &Data) == 0, "Failed to load WinSock");
    Enforce(LOBYTE(Data.wVersion) == 2 && HIBYTE(Data.wVersion) == 2 , "Version 2.2 of Winsock not available.");
    Enforce(IPAddress != nullptr && Port != nullptr, "Invalid IP address or port number");

    addrinfo Hints{};
    addrinfo* InfoLinkedList = nullptr;

	// TCP sockets because UDP is for people who like shooting themselves in the foot!
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE; 

    Enforce(getaddrinfo(IPAddress, Port, &Hints, &InfoLinkedList) == 0, "Failed to obtain address info");

    int32_t ListenFD = -1;
    for (addrinfo* Info = InfoLinkedList; Info != nullptr; Info = Info->ai_next)
    {
        if ((ListenFD = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol)) == SOCKET_ERROR)
        {
            std::cerr << "Invalid socket, trying the next one...\n";
            continue;
        }

        char Options = 1;
        Enforce(setsockopt(ListenFD, SOL_SOCKET, SO_REUSEADDR, &Options, sizeof(Options)) == 0, "Failed to set socket options");

        if (bind(ListenFD, Info->ai_addr, Info->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(ListenFD) == 0;
            std::cerr << "Failed to bind socket, trying the next one...\n";
            continue;
        }

        Enforce(Info != nullptr, "Error occured during port binding");
        break;
    }

	Enforce(ListenFD != -1, "Failed to get valid listening socket");
	Enforce(listen(ListenFD, MaxConnections) == 0, "Failed to listen for incoming connections");
	freeaddrinfo(InfoLinkedList);
	InfoLinkedList = nullptr;
	
    ConnectionList = new pollfd[MaxConnections]();
	ConnectionList[0].fd = ListenFD;
	ConnectionList[0].events = POLLIN;
	ConnectionCount++;

    MessageBuffer = new char[BufferSize * MaxConnections]();
}

void HTTP::Server::Run()
{
    std::cout << "Launching server. Waiting for new connections...\n";
	
	while (true) 
    {
        // Start polling with -1 timeout to poll forever
        Enforce(WSAPoll(ConnectionList, ConnectionCount, -1) != SOCKET_ERROR, "Error occured during polling");

        for (uint8_t ConnectionIndex = 0; ConnectionIndex < ConnectionCount; ConnectionIndex++) 
        {
            if (ConnectionList[ConnectionIndex].revents & (POLLIN | POLLHUP)) 
            {
                if (ConnectionIndex == 0)
					HandleNewConnection();
                else
					HandleClientData(ConnectionIndex);
            }
        }
    }
}

void HTTP::Server::HandleNewConnection()
{
	sockaddr_storage RemoteAddr{};
	socklen_t AddrLen = sizeof(RemoteAddr);
	int32_t NewFD = -1;
	char RemoteIP[INET6_ADDRSTRLEN] = "";

	NewFD = accept(ConnectionList[0].fd, reinterpret_cast<sockaddr*>(&RemoteAddr), &AddrLen);
	if (NewFD == -1)
	{
		std::cerr << "Failed to accept socket connection\n";
	}
	else
	{
		if (ConnectionCount >= 32)
		{
			std::cout << "No room in poll buffer to add new connection\n";
		}
		else
		{
			// Add the new socket to the poll
			ConnectionList[ConnectionCount].fd = NewFD;
			ConnectionList[ConnectionCount].events = POLLIN;
			ConnectionList[ConnectionCount].revents = 0;

			ConnectionCount++;

			// Find out the IP address string of the socket
			sockaddr_in* SA4 = nullptr;
			sockaddr_in6* SA6 = nullptr;
			void* Src = nullptr;

			switch (RemoteAddr.ss_family) 
			{
			case AF_INET:
				SA4 = reinterpret_cast<sockaddr_in*>(&RemoteAddr);
				Src = &(SA4->sin_addr);
				break;
			case AF_INET6:
				SA6 = reinterpret_cast<sockaddr_in6*>(&RemoteAddr);
				Src = &(SA6->sin6_addr);
				break;
			default:
				std::cerr << "Failed to detect either IPV4 or IPV6\n";
				break;
			}

			Enforce(Src != nullptr, "Src is null");
			inet_ntop(RemoteAddr.ss_family, Src, RemoteIP, sizeof(RemoteIP));

			std::cout << "New connection from " << RemoteIP;
			std::cout << " on socket " << NewFD << "\n";
		}
	}
}

void HTTP::Server::HandleClientData(uint8_t Index)
{
	char* Message = MessageBuffer + BufferSize * Index;
	int32_t NumBytes = recv(ConnectionList[Index].fd, Message, BufferSize, 0);
	int32_t SenderFD = ConnectionList[Index].fd;

	Enforce(NumBytes <= BufferSize, "Message was too big to fit in the buffer");

	if (NumBytes <= 0)
	{
		// Got error or connection closed by client
		if (NumBytes == 0)
			std::cerr << "Socket " << SenderFD << " closed the connection\n";
		else
			std::cerr << "Error receiving message from socket " << SenderFD << "\n";

		RemoveConnection(Index);
		return;
	}
	
	std::string RawData(Message, NumBytes);
    Request Req = ParseRequest(RawData);
    
    // Find matching route
    std::string Key = Req.Method + ":" + Req.URI;
    auto It = Routes.find(Key);
    
    Response Res{};
    if (It != Routes.end())
    {
        Res = It->second(Req);
    }
    else
    {
        //@ todo Create a 404 Response
        //Res = CreateResponse(404, "404 Not Found", "text/html");
    }
    
    SendResponse(ConnectionList[Index].fd, Res);
}

HTTP::Request HTTP::Server::ParseRequest(const std::string& RawData)
{
    Request Req{};

    std::vector<std::string> Parts = SplitByDelimiter(RawData, "\r\n\r\n");
    if (Parts.empty()) return Req;

    std::vector<std::string> Lines = SplitByDelimiter(Parts[0], "\r\n");
    if (Lines.empty()) return Req;

    // Parse request line
    std::vector<std::string> RequestLine = SplitByDelimiter(Lines[0], " ");
    if (RequestLine.size() >= 3)
    {
        Req.Method = RequestLine[0];
        Req.URI = RequestLine[1];
        Req.Version = RequestLine[2];
    }

    // Parse headers
    for (uint64_t i = 1; i < Lines.size(); i++)
    {
        uint64_t ColonPosition = Lines[i].find(':');
        if (ColonPosition != std::string::npos)
        {
            std::string Key = Trim(Lines[i].substr(0, ColonPosition));
            std::string Value = Trim(Lines[i].substr(ColonPosition + 1));
            Req.Headers[Key] = Value;
        }
    }

    // Parse body
    if (Parts.size() >= 2)
    {
        Req.Body = Parts[1];
    }

    return Req;
}

void HTTP::Server::SendResponse(uint8_t ClientFD, const Response& Res)
{
    std::string ResponseString;// = Resp.ToString();
    uint32_t TotalSent = 0;
    uint32_t Remaining = ResponseString.size();
    
    while (Remaining > 0)
    {
        uint32_t Sent = send(ClientFD, ResponseString.c_str() + TotalSent, Remaining, 0);
        if (Sent == SOCKET_ERROR)
        {
            std::cerr << "Failed to send response: " << WSAGetLastError() << "\n";
            return;
        }
		
        TotalSent += Sent;
        Remaining -= Sent;
    }
}

void HTTP::Server::RemoveConnection(uint8_t Index)
{
	if (Index <= 0 || Index >= MaxConnections)
		return;
	
	
	std::memset(MessageBuffer + BufferSize * Index, 0, BufferSize);
}

void HTTP::Server::AddRoute(const std::string& Method, const std::string& Path, std::function<Response(const Request&)> Handler)
{
    std::string Key = Method + ":" + Path;
    Routes[Key] = Handler;
    std::cout << "Added route: " << Method << " " << Path << "\n";
}

HTTP::Server::~Server()
{
    Enforce(closesocket(ConnectionList[0].fd) == 0, "Failed to close the listening socket");

    if (ConnectionList)
    {
        delete[] ConnectionList;
    }

    if (MessageBuffer)
    {
        delete[] MessageBuffer;
    }
	
    WSACleanup();
}
