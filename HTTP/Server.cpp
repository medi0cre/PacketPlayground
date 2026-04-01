#include <iostream>
#include <vector>
#include <cstdint>
#include <unordered_map>

#include "Server.h"

void Enforce(bool Condition, const char* Message)
{
    if (!Condition)
    {
        std::cerr << Message << "\n";
        std::abort();
    }
}

std::vector<std::string> SplitByDelimiter(const std::string& Input, const std::string& Delimiter) 
{
    std::vector<std::string> Result{};
    uint64_t Start = 0;

    if (Delimiter.empty()) 
    {
        Result.push_back(Input);
        return Result;
    }

    while (true) 
    {
        uint64_t End = Input.find(Delimiter, Start);

        if (End == std::string::npos) 
        {
            Result.emplace_back(Input.substr(Start));
            break;
        }

        Result.emplace_back(Input.substr(Start, End - Start));
        Start = End + Delimiter.length();
    }

    return Result;
}

HTTP::Server::Server(const char* IPAddress, const char* Port)
{
    WSADATA Data{};
    Enforce(WSAStartup(MAKEWORD(2, 2), &Data) == 0, "Failed to load WinSock");
    Enforce(LOBYTE(Data.wVersion) == 2 && HIBYTE(Data.wVersion) == 2 , "Version 2.2 of Winsock not available.");
    Enforce(IPAddress != nullptr && Port != nullptr, "Invalid IP address or port number");

    addrinfo Hints{};
    addrinfo* InfoLinkedList = nullptr;

    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE; 

    Enforce(getaddrinfo(IPAddress, Port, &Hints, &InfoLinkedList) == 0, "Failed to obtain address info");

    int ListenFD = -1;
    for (addrinfo* Info = InfoLinkedList; Info != nullptr; Info = Info->ai_next)
    {
        if ((ListenFD = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol)) == -1)
        {
            std::cerr << "Invalid socket, trying the next one...\n";
            continue;
        }

        char Options = 1;
        Enforce(setsockopt(ListenFD, SOL_SOCKET, SO_REUSEADDR, &Options, sizeof(int)) != -1, "Failed to set socket options");

        if (bind(ListenFD, Info->ai_addr, Info->ai_addrlen) == -1)
        {
            closesocket(ListenFD);
            std::cerr << "Failed to bind socket, trying the next one...\n";
            continue;
        }

        Enforce(Info != nullptr, "Error occured during port binding");
        break;
    }

	Enforce(ListenFD != -1, "Failed to get valid listening socket");
	Enforce(listen(ListenFD, 32) != -1, "Failed to listen for incoming connections");
	freeaddrinfo(InfoLinkedList);

    ConnectionList = new pollfd[32];
	ConnectionList[0].fd = ListenFD;
	ConnectionList[0].events = POLLIN;
	ConnectionCount++;

    MessageBuffer = new char[32768 * 32];
}

void HTTP::Server::Run()
{
    std::cout << "Launching server. Waiting for new connections...\n";
	
	while (true) 
    {
        // Start polling with -1 timeout to poll forever
        Enforce(WSAPoll(ConnectionList, ConnectionCount, -1) != -1, "Error occured during polling");

        for (int i = 0; i < ConnectionCount; i++) 
        {
            if (ConnectionList[i].revents & (POLLIN | POLLHUP)) 
            {
                if (i == 0)
                {
                    // Handle new connections
                    sockaddr_storage RemoteAddr{};
                    socklen_t AddrLen = sizeof(RemoteAddr);
                    int NewFD = -1;
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
                else
                {
                    char* Message = MessageBuffer + 32768 * i;
                    int NumBytes = recv(ConnectionList[i].fd, Message, 32768, 0);
                    int SenderFD = ConnectionList[i].fd;

                    Enforce(NumBytes <= 32768, "Message was too big to fit in the buffer");

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

                        closesocket(ConnectionList[i].fd);
                        ConnectionList[i] = ConnectionList[ConnectionCount - 1];
                        ConnectionCount--;
                        i--;
                    }
                    else
                    {
                        // Received some good data from the client
						bool Valid = true;
                        std::string RawData(Message, NumBytes);
                        std::vector<std::string> HeadAndBody = SplitByDelimiter(RawData, "\r\n\r\n");
						if (HeadAndBody.size() != 2) { Valid = false; }

                        std::vector<std::string> HeaderLines = SplitByDelimiter(HeadAndBody[0], "\r\n");
                        std::unordered_map<std::string, std::string> Headers{};

                        std::vector<std::string> RequestLine = SplitByDelimiter(HeaderLines[0], " ");
                        if (RequestLine.size() != 3 || RequestLine[0] != "GET") { Valid = false; } // Only deal with GET requests for now

                        for (uint16_t i = 1; i < HeaderLines.size(); i++)
                        {
                            uint64_t Position = HeaderLines[i].find(":", 0);
							if (Position == std::string::npos) { Valid = false; break; }

                            std::string Key = HeaderLines[i].substr(0, Position);
                            std::string Value = HeaderLines[i].substr(Position + 1);

                            Headers[Key] = Value;
                        }
						
						std::string Response = "";
						
						if (!Valid)
						{
							Response = "HTTP/1.1 400 Bad Request\r\n"
									   "Content-Length: 0\r\n"
									   "\r\n";
						}
						else
						{
							Response = "HTTP/1.1 200 OK\r\n"
									   "Content-Length: 0\r\n"
									   "\r\n";
						}
						
						Enforce(send(ConnectionList[i].fd, Response.c_str(), Response.size(), 0) != SOCKET_ERROR, "Failed to send data to the client");
                    }
                }
            }
        }
    }
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
