#include <iostream>
#include <sstream>

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

    SOCKET ListenFD = INVALID_SOCKET;
    for (addrinfo* Info = InfoLinkedList; Info != nullptr; Info = Info->ai_next)
    {
        if ((ListenFD = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol)) == INVALID_SOCKET)
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

    Enforce(ListenFD != INVALID_SOCKET, "Failed to get valid listening socket");
    Enforce(listen(ListenFD, MaxConnections) == 0, "Failed to listen for incoming connections");
    freeaddrinfo(InfoLinkedList);
    InfoLinkedList = nullptr;

    Connection ListeningSocket{};
    ListeningSocket.Socket.fd = ListenFD;
    ListeningSocket.Socket.events = POLLIN;
    ConnectionList.emplace_back(ListeningSocket);
    Enforce(ConnectionList.size() == 1, "There should not be any other connections other than the listening socket");
}

void HTTP::Server::Run()
{
    std::cout << "Launching server. Waiting for new connections...\n";

    while (true) 
    {
        std::vector<WSAPOLLFD> PollFDList{};
        PollFDList.reserve(ConnectionList.size());

        for (int i = 0; i < ConnectionList.size(); i++)
        {
            PollFDList.emplace_back(ConnectionList[i].Socket);
		}

		// Start polling with -1 timeout to poll forever
        Enforce(PollFDList.size() == ConnectionList.size(), "Mapping wrong between PollFDList and ConnectionList");
		Enforce(WSAPoll(PollFDList.data(), PollFDList.size(), -1) != SOCKET_ERROR, "Error occured during polling");

        for (int ConnectionIndex = ConnectionList.size() - 1; ConnectionIndex >= 0; ConnectionIndex--) 
        {
            if (PollFDList[ConnectionIndex].revents & (POLLIN | POLLHUP)) 
            {
                if (ConnectionIndex == 0)
				{
					HandleNewConnection();
					Enforce(ConnectionList.size() == PollFDList.size() + 1 || ConnectionList.size() == MaxConnections, "Wrong handling of new connection");
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
    SOCKET NewFD = INVALID_SOCKET;
    char RemoteIP[INET6_ADDRSTRLEN]{};

    NewFD = accept(ConnectionList[0].Socket.fd, reinterpret_cast<sockaddr*>(&RemoteAddr), &AddrLen);
    Enforce(NewFD != INVALID_SOCKET, "Invalid file descriptor obtained from accept() call");
	
    if (ConnectionList.size() >= MaxConnections)
    {
		std::cout << "No room in poll buffer to add new connection\n";
        Enforce(closesocket(NewFD) == 0, "Failed to close socket after reaching max capacity");
        return;
    }
    else
    {
		// Add the new socket to the poll
        Connection Client{};
        Client.Socket.fd = NewFD;
        Client.Socket.events = POLLIN;
        Client.Socket.revents = 0;
        Client.State = AcceptingHeaders;
        ConnectionList.emplace_back(Client);

		// Find out the IP address string of the socket
		sockaddr_in* SA4 = nullptr;
		sockaddr_in6* SA6 = nullptr;
		void* Src = nullptr;

		switch (RemoteAddr.ss_family) 
		{
		case AF_INET:
        {
            SA4 = reinterpret_cast<sockaddr_in*>(&RemoteAddr);
			Src = &(SA4->sin_addr);
			break;
        }
		case AF_INET6:
        {
			SA6 = reinterpret_cast<sockaddr_in6*>(&RemoteAddr);
			Src = &(SA6->sin6_addr);
			break;
        }
		default:
			Enforce(false, "Failed to detect either IPV4 or IPV6");
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
    Enforce(Index >= 0 && Index < ConnectionList.size(), "Invalid connection index");

    char Data[BufferSize]{};
    int NumBytes = recv(ConnectionList[Index].Socket.fd, Data, BufferSize, 0);
    SOCKET SenderFD = ConnectionList[Index].Socket.fd;
    Enforce(SenderFD != INVALID_SOCKET, "Socket is invalid and cannot be used");
	    
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
            std::cerr << "Error Code: " << WSAGetLastError() << "\n";
		}
		
        Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket inside HandleClientData()");
        ConnectionList.erase(ConnectionList.begin() + Index);
        return;
    }
	
	ConnectionList[Index].Buffer += std::string(Data, NumBytes);

    // Check for suspicious connections
    if (ConnectionList[Index].Buffer.length() > BufferSize || ConnectionList[Index].ClientRequest.Body.length() > BufferSize)
    {
        std::cerr << "Total message size exceeded 32 kibibytes, potential DDoS attack\n";
        std::cerr << "Removing malicious socket " << std::to_string(ConnectionList[Index].Socket.fd) << " from the list\n";
        Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket after potential DDoS attack");
        ConnectionList.erase(ConnectionList.begin() + Index);
        return;
    }

    // Parsing state machine
	while (true)
	{
		switch (ConnectionList[Index].State)
		{
		case AcceptingHeaders:
		{
			// Look for a blank line
			size_t BlankLinePosition = ConnectionList[Index].Buffer.find("\r\n\r\n");
			if (BlankLinePosition == std::string::npos)
			{
				return; // No blank line found, still getting header data
			}

			// Store and remove head from the buffer
			std::string Head = ConnectionList[Index].Buffer.substr(0, BlankLinePosition);
			ConnectionList[Index].Buffer.erase(0, BlankLinePosition + 4);

			// Organize the data before moving on to body
			std::vector<std::string> RequestAndHeaders = SplitByDelimiter(Head, "\r\n");
			if (RequestAndHeaders.empty())
			{
				ConnectionList[Index].State = Faulty;
				break;
			}
			
			std::stringstream Stream(RequestAndHeaders[0]);
			std::vector<std::string> RequestLine{};
			std::string Token = "";

			while (Stream >> Token)
			{
				RequestLine.emplace_back(Token);
			}

			if (RequestLine.size() != 3)
			{
				ConnectionList[Index].State = Faulty;
				break;
			}

			ConnectionList[Index].ClientRequest.Method = RequestLine[0];
			ConnectionList[Index].ClientRequest.URI = RequestLine[1];
			ConnectionList[Index].ClientRequest.Version = RequestLine[2];

			for (int i = 1; i < RequestAndHeaders.size(); i++)
			{
				size_t ColonPosition = RequestAndHeaders[i].find(':');
				if (ColonPosition != std::string::npos)
				{
					std::string Key = Trim(RequestAndHeaders[i].substr(0, ColonPosition));
					std::string Value = Trim(RequestAndHeaders[i].substr(ColonPosition + 1));
					ConnectionList[Index].ClientRequest.Headers[LowerCase(Key)] = Value;
				}
			}

			// Try to find the "Content-Length" header if possible before moving onto the next state
			long long ContentLength = 0;
			auto LengthIterator = ConnectionList[Index].ClientRequest.Headers.find("content-length");
		
			if (LengthIterator != ConnectionList[Index].ClientRequest.Headers.end())
			{
				// Content-Length header found
				try
				{
					ContentLength = std::stoll(LengthIterator->second);
					if (ContentLength < 0)
					{
						ConnectionList[Index].State = Faulty;
						break;
					}

					ConnectionList[Index].BodyLength = ContentLength;
				}
				catch (const std::exception& Exception)
				{
					std::cerr << "Invalid Content-Length: " << Exception.what() << "\n";
					ConnectionList[Index].State = Faulty;
					break;
				}

				ConnectionList[Index].State = AcceptingBody;
			} 
			else
			{
				// GET request or header not found
				ConnectionList[Index].State = ProcessingRequest;
			}
	 
			break;
		}
		case AcceptingBody:    
		{
			if (ConnectionList[Index].BodyLength == 0)
			{
				ConnectionList[Index].State = ProcessingRequest;
				break;
			}

			if (ConnectionList[Index].BodyLength > ConnectionList[Index].Buffer.length())
			{
				return; // Still getting body data
			}

			// Body data fully received
			ConnectionList[Index].ClientRequest.Body = ConnectionList[Index].Buffer.substr(0, ConnectionList[Index].BodyLength);
			ConnectionList[Index].Buffer = "";
			ConnectionList[Index].State = ProcessingRequest;

			break;
		}
		case ProcessingRequest:
		{
			ResponseBuilder Builder{};
			std::string Method = LowerCase(ConnectionList[Index].ClientRequest.Method);
			std::string Version = ConnectionList[Index].ClientRequest.Version;

			if (Method != "get" 
				&& Method != "put" 
				&& Method != "post" 
				&& Method != "patch" 
				&& Method != "delete")
			{
				SendResponse(ConnectionList[Index].Socket.fd, Builder.BadRequest().Build());
				Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket after bad request");	
				ConnectionList.erase(ConnectionList.begin() + Index);
				return;
			}
			
			if (Version != "HTTP/1.1" && Version != "HTTP/1.0")
			{
				SendResponse(ConnectionList[Index].Socket.fd, Builder.BadRequest().Build());
				Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket after bad request");	
				ConnectionList.erase(ConnectionList.begin() + Index);
				return;
			}

			if (Version == "HTTP/1.1" 
			&& ConnectionList[Index].ClientRequest.Headers.find("host") == ConnectionList[Index].ClientRequest.Headers.end())
			{
				ConnectionList[Index].State = Faulty;
				break;
			}

			// Try to find the requested route
			std::string Key = Method + ":" + ConnectionList[Index].ClientRequest.URI;
			auto RouteIterator = Routes.find(Key);
		
			if (RouteIterator != Routes.end())
			{
				SendResponse(ConnectionList[Index].Socket.fd, RouteIterator->second(ConnectionList[Index].ClientRequest));
				Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket after finding route");
				std::cerr << "Found the requested route, sending response\n";
				ConnectionList.erase(ConnectionList.begin() + Index);
				return;
			}
			else
			{
				SendResponse(ConnectionList[Index].Socket.fd, Builder.NotFound().Build());
				Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket after not finding route");
				std::cerr << "Failed to find the requested resource, sending 404 error\n";
				ConnectionList.erase(ConnectionList.begin() + Index);
				return;
			}

			break;
		}
		case Faulty:
		{
			// Remove the connection from the list
			Enforce(closesocket(ConnectionList[Index].Socket.fd) == 0, "Failed to close socket inside HandleClientData()");
			std::cerr << "Faulty connection at socket " << std::to_string(ConnectionList[Index].Socket.fd) << "\n";
			ConnectionList.erase(ConnectionList.begin() + Index);
			return;
		}
		default:
			Enforce(false, "Parsing state should never be null unless there is a bug");
			return;
		}
	}
}

std::string HTTP::Server::CPPString(const Response& Res)
{
	Enforce(Res.Version == "HTTP/1.1" || Res.Version == "HTTP/1.0", "HTTP version not supported");
	Enforce(Res.Headers.size() != 0, "Headers are empty");

    std::string Result = "";
    Result += Res.Version + " " + std::to_string(Res.StatusCode) + " " + Res.Status + "\r\n";
    
    for (const auto& Header: Res.Headers)
    {
        Result += Header.first + ": " + Header.second + "\r\n";
    }

    Result += "\r\n" + Res.Body;
    return Result;
}

void HTTP::Server::SendResponse(SOCKET ClientFD, const Response& Res)
{
    std::string ResponseString = CPPString(Res);
	Enforce(ResponseString.size() != 0, "Empty response");
	Enforce(ClientFD != INVALID_SOCKET, "Invalid client socket");

    size_t TotalSent = 0;
    size_t Remaining = ResponseString.size();
    
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

	Enforce(Remaining == 0 && TotalSent == ResponseString.size(), "Response was not fully sent");
}

void HTTP::Server::AddRoute(std::string Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher)
{
	Method = LowerCase(Method);
	Enforce(Method == "get" || Method == "post" || Method == "patch" || Method == "put" || Method == "delete", "Invalid method provided");
	Enforce(Path.size() != 0, "Invalid path provided");

    std::string Key = Method + ":" + Path;
    Routes[Key] = Dispatcher;
    std::cout << "Added route: " << Method << " " << Path << "\n";
}

HTTP::Server::~Server()
{
	for (int i = 0; i < ConnectionList.size(); i++)
	{
		if (ConnectionList[i].Socket.fd != INVALID_SOCKET)
		{
			closesocket(ConnectionList[i].Socket.fd);
		}
	}

    WSACleanup();
}

HTTP::Response HTTP::ResponseBuilder::Build()
{
    Res.Headers["Connection"] = "Close";
    Res.Headers["Server"] = "PacketPlayground";
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

	
	Reset();
	Version("HTTP/1.1");
	StatusCode(404);
	Status("Not Found");
	HTML();
	Body(NotFoundBody);

	return *this;
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

	
	Reset();
	Version("HTTP/1.1");
	StatusCode(400);
	Status("Bad Request");
	HTML();
	Body(BadRequestBody);

	return *this;
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
