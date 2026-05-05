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

	// TCP sockets because UDP is for people who enjoy 
	// shooting themselves in the foot with military precision!
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE; 

    Enforce(getaddrinfo(IPAddress, Port, &Hints, &InfoLinkedList) == 0, "Failed to obtain address info");

    int ListenFD = -1;
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
            closesocket(ListenFD);
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

    Connection ListeningSocket{};
    ListeningSocket.Socket.fd = ListenFD;
    ListeningSocket.Socket.events = POLLIN;
    ConnectionList.emplace_back(ListeningSocket);
}

void HTTP::Server::Run()
{
    std::cout << "Launching server. Waiting for new connections...\n";

    while (true) 
    {
        std::vector<pollfd> PollFDList{};
		PollFDList.reserve(ConnectionList.size());

        for (int i = 0; i < ConnectionList.size(); i++)
		{
            PollFDList.emplace_back(ConnectionList[i].Socket);
		}

        // Start polling with -1 timeout to poll forever
        Enforce(WSAPoll(PollFDList.data(), PollFDList.size(), -1) != SOCKET_ERROR, "Error occured during polling");

        for (int ConnectionIndex = ConnectionList.size() - 1; ConnectionIndex >= 0; ConnectionIndex--) 
        {
            if (PollFDList[ConnectionIndex].revents & (POLLIN | POLLHUP)) 
            {
                if (ConnectionIndex == 0)
				{
					HandleNewConnection();
				}
                else
				{
					HandleClientData(ConnectionIndex);
				}
            }
        }
    }
}

void HTTP::Server::HandleNewConnection()
{
	sockaddr_storage RemoteAddr{};
	socklen_t AddrLen = sizeof(RemoteAddr);
	int NewFD = -1;
	char RemoteIP[INET6_ADDRSTRLEN] = "";

	NewFD = accept(ConnectionList[0].Socket.fd, reinterpret_cast<sockaddr*>(&RemoteAddr), &AddrLen);
	Enforce(NewFD != -1, "Invalid file descriptor obtained from accept() call");
	
	if (ConnectionList.size() >= MaxConnections)
	{
		std::cout << "No room in poll buffer to add new connection\n";
	}
	else
	{
		// Add the new socket to the poll
        Connection Client{};
        Client.Socket.fd = NewFD;
        Client.Socket.events = POLLIN;
        Client.Socket.revents = 0;
        ConnectionList.emplace_back(Client);

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

void HTTP::Server::HandleClientData(int Index)
{
	char Message[BufferSize];
	int NumBytes = recv(ConnectionList[Index].Socket.fd, Message, BufferSize, 0);
	int SenderFD = ConnectionList[Index].Socket.fd;

	if (NumBytes <= 0)
	{
		// Got error or connection closed by client
		if (NumBytes == 0)
		{
			std::cerr << "Socket " << SenderFD << " closed the connection\n";
		}
		else
		{
			std::cerr << "Error receiving message from socket " << SenderFD << "\n";
		}

        // Remove the connection from the list
		Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket inside HandleClientData()");
        ConnectionList.erase(ConnectionList.begin() + Index);
		return;
	}

    ResponseBuilder Builder{};

    // Add the data to the connection buffer
    std::string MessageString = std::string(Message, NumBytes);
    ConnectionList[Index].MessageBuffer += MessageString;
    Enforce(ConnectionList[Index].MessageBuffer.length() <= BufferSize, "Total size of message is bigger than 32 kibibytes, potential DDoS attack!");

    // Split the headers and body
    std::vector<std::string> HeadersAndBody = SplitByDelimiter(MessageString, "\r\n\r\n");
    if (HeadersAndBody.size() != 2)
    {
        SendResponse(ConnectionList[Index].Socket.fd, Builder.BadRequest().Build());
        return;
    }

    // Split the request line from the headers
    std::vector<std::string> RequestAndHeaders = SplitByDelimiter(HeadersAndBody[0], "\r\n");
    if (RequestAndHeaders.empty())
    {
        SendResponse(ConnectionList[Index].Socket.fd, Builder.BadRequest().Build());
        return;
    }

    // Get the data from the request line
    std::vector<std::string> RequestLine = SplitByDelimiter(RequestAndHeaders[0], " ");
    if (RequestLine.size() != 3)
    {
        SendResponse(ConnectionList[Index].Socket.fd, Builder.BadRequest().Build());
        return;
    }

    // Make the request struct, useless now but might become necessary as project scales
    Request ClientRequest{};
    ClientRequest.Method = RequestLine[0];
    ClientRequest.URI = RequestLine[1];
    ClientRequest.Version = RequestLine[2];
    ClientRequest.Body = HeadersAndBody[1];

    for (int i = 1; i < RequestAndHeaders.size(); i++)
    {
        int ColonPosition = RequestAndHeaders[i].find(':');
        if (ColonPosition != std::string::npos)
        {
            std::string Key = Trim(RequestAndHeaders[i].substr(0, ColonPosition));
            std::string Value = Trim(RequestAndHeaders[i].substr(ColonPosition + 1));
            ClientRequest.Headers[Key] = Value;
        }
    }
    
    // Try to find the requested route
    std::string Key = ClientRequest.Method + ":" + ClientRequest.URI;
    auto Iterator = Routes.find(Key);
    
    if (Iterator != Routes.end())
    {
        SendResponse(ConnectionList[Index].Socket.fd, Iterator->second(ClientRequest));
        return;
    }
    else
    {
        SendResponse(ConnectionList[Index].Socket.fd, Builder.NotFound().Build());
        return;
    }
}

std::string HTTP::Server::CPPString(const Response& Res)
{
    std::string Result = "";
    Result += Res.Version + " " + std::to_string(Res.StatusCode) + " " + Res.Status + "\r\n";
    
    for (const auto& Header: Res.Headers)
        Result += Header.first + ": " + Header.second + "\r\n";

    Result += "\r\n" + Res.Body;
    return Result;
}

void HTTP::Server::SendResponse(int ClientFD, const Response& Res)
{
    std::string ResponseString = CPPString(Res);
    int TotalSent = 0;
    int Remaining = ResponseString.size();
    
    while (Remaining > 0)
    {
        int Sent = send(ClientFD, ResponseString.c_str() + TotalSent, Remaining, 0);
        if (Sent == SOCKET_ERROR)
        {
            std::cerr << "Failed to send response: " << WSAGetLastError() << "\n";
            return;
        }
		
        TotalSent += Sent;
        Remaining -= Sent;
    }
}

void HTTP::Server::AddRoute(const std::string& Method, const std::string& Path, std::function<Response(const Request&)> Handler)
{
    std::string Key = Method + ":" + Path;
    Routes[Key] = Handler;
    std::cout << "Added route: " << Method << " " << Path << "\n";
}

HTTP::Server::~Server()
{
    Enforce(closesocket(ConnectionList[0].Socket.fd) == 0, "Failed to close the listening socket");
    Enforce(WSACleanup() == 0, "Failed to clean up winsock API");
}

HTTP::Response& HTTP::ResponseBuilder::Build()
{
	Res.Headers["Content-Length"] = std::to_string(Res.Body.size());
    return Res;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Reset()
{
	Res = {};
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Version(const std::string& Version)
{
	Res.Version = Version;
	return *this;
} 

HTTP::ResponseBuilder& HTTP::ResponseBuilder::StatusCode(int StatusCode)
{
	Res.StatusCode = StatusCode;
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Status(const std::string& Status)
{
	Res.Status = Status;
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Body(const std::string& Body)
{
	Res.Body = Body;
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Header(const std::string& Key, const std::string& Value)
{
	Res.Headers[Key] = Value;
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::NotFound()
{
    const std::string NotFoundBody = "<html>"
                                       "<head><title>404 Not Found</title></head>"
                                       "<body>"
                                         "<h1>Not Found</h1>"
                                         "<p>The requested resource was not found on this server.</p>"
                                       "</body>"
                                     "</html>";

	
	return Reset()
		.Version("HTTP/1.1")
		.StatusCode(404)
		.Status("Not Found")
		.HTML()
		.Header("Connection", "close")
		.Body(NotFoundBody);
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::BadRequest()
{
    const std::string BadRequestBody = "<html>"
                                         "<head><title>400 Bad Request</title></head>"
                                         "<body>"
                                           "<h1>400 Bad Request</h1>"
                                           "<p>Your browser sent a request that the server could not understand</p>"
                                         "</body>"
                                       "</html>";

	
	return Reset()
		.Version("HTTP/1.1")
		.StatusCode(400)
		.Status("Bad Request")
		.HTML()
		.Header("Connection", "close")
		.Body(BadRequestBody);
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::OK()
{
	Reset();
	Version("HTTP/1.1");
    StatusCode(200);
    Status("OK");
	
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::JSON()
{
    Header("Content-Type", "application/json");
	return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::HTML()
{
    Header("Content-Type", "text/html");
	return *this;
}
