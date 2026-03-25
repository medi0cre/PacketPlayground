#include <iostream>
#include <vector>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>

void Assert(bool Condition, const std::string& Message) 
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

int main(int argc, char* argv[])
{
    WSADATA Data{};
    addrinfo Hints{};
    addrinfo* Info = nullptr;
    addrinfo* InfoLL = nullptr;
    sockaddr_storage ClientAddr{};
    pollfd* PFDS = new pollfd[32]; 

    int Status = 0, ListenFD = 0, DataFD = 0, FDCount = 0; 
    int AddrSize = sizeof(ClientAddr);
    char IPString[INET6_ADDRSTRLEN] = "";
    char Options = 1;
    void* SinAddr = nullptr;

    Assert(WSAStartup(MAKEWORD(2, 2), &Data) == 0, "Failed to load WinSock");

    if (LOBYTE(Data.wVersion) != 2 || HIBYTE(Data.wVersion) != 2)
    {
        std::cerr << "Version 2.2 of Winsock not available.\n";
        WSACleanup();
        delete[] PFDS;
        std::exit(2);
    }

    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE;

    //Assert(getaddrinfo(argv[1], argv[2], &Hints, &InfoLL) == 0, "Failed to obtain address info");
    Assert(getaddrinfo("127.0.0.1", "8080", &Hints, &InfoLL) == 0, "Failed to obtain address info");

    for (Info = InfoLL; Info != nullptr; Info = Info->ai_next) 
    {
        if ((ListenFD = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol)) == -1)
        {
            std::cerr << "socket() error\n";
            continue;
        }

        Assert(setsockopt(ListenFD, SOL_SOCKET, SO_REUSEADDR, &Options, sizeof(int)) != -1, "Failed to set socket options");

        if (bind(ListenFD, Info->ai_addr, Info->ai_addrlen) == -1) 
        {
            closesocket(ListenFD);
            std::cerr << "bind() error\n";
            continue;
        }

        break;
    }

    freeaddrinfo(InfoLL);

    Assert(Info != nullptr, "Error occured during port binding");

    // Queue up to 32 pending connections
    Assert(listen(ListenFD, 32) != -1, "Failed to listen for incoming connections");

    PFDS[0].fd = ListenFD;
    PFDS[0].events = POLLIN;
    FDCount++;

    Assert(FDCount == 1, "There are other sockets aside from the listening socket");

    std::cout << "Waiting for new connections\n";

    while (true) 
    {
        // Start polling with -1 timeout to poll forever
        Assert(WSAPoll(PFDS, FDCount, -1) != -1, "Error occured during polling");

        for (int i = 0; i < FDCount; i++) 
        {
            if (PFDS[i].revents & (POLLIN | POLLHUP)) 
            {
                if (PFDS[i].fd == ListenFD) 
                {
                    // Handle new connections
                    sockaddr_storage RemoteAddr{};
                    socklen_t AddrLen = sizeof(RemoteAddr);
                    int NewFD = -1;  
                    char RemoteIP[INET6_ADDRSTRLEN] = "";

                    NewFD = accept(ListenFD, reinterpret_cast<sockaddr*>(&RemoteAddr), &AddrLen);
                    if (NewFD == -1)
                    {
                        std::cerr << "Failed to accept socket connection\n";
                    }
                    else
                    {
                        if (FDCount >= 32) 
                        {
                            std::cout << "No room in poll buffer to add new connection\n";
                        }
                        else 
                        {
                            // Add the new socket to the poll
                            PFDS[FDCount].fd = NewFD;
                            PFDS[FDCount].events = POLLIN;
                            PFDS[FDCount].revents = 0;

                            FDCount++;

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

                            Assert(Src != nullptr, "Src is null");
                            inet_ntop(RemoteAddr.ss_family, Src, RemoteIP, sizeof(RemoteIP));

                            std::cout << "New connection from " << RemoteIP;
                            std::cout << " on socket " << NewFD << "\n";
                        }
                    }
                }
                else
                {
                    char Message[1024];
                    int NumBytes = recv(PFDS[i].fd, Message, sizeof(Message), 0);
                    int SenderFD = PFDS[i].fd;

                    Assert(NumBytes <= 1024, "Message was too big to fit in the buffer");

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

                        closesocket(PFDS[i].fd); 
                        PFDS[i] = PFDS[FDCount - 1];
                        FDCount--;
                        i--;
                    }
                    else
                    {
                        // Received some good data from the client
                        // @todo: Implement an HTTP parser to check client requests
                        std::string RawData(Message, NumBytes);
                        std::vector<std::string> HeadAndBody = SplitByDelimiter(RawData, "\r\n\r\n");
                        Assert(HeadAndBody.size() == 2, "HTTP request format is invalid");

                        std::vector<std::string> HeaderLines = SplitByDelimiter(HeadAndBody[0], "\r\n");
                        std::unordered_map<std::string, std::string> Headers{};

                        std::vector<std::string> RequestLine = SplitByDelimiter(HeaderLines[0], " ");
                        Assert(RequestLine.size() == 3 && RequestLine[0] == "GET", "Invalid request line");

                        for (uint16_t i = 1; i < HeaderLines.size(); i++)
                        {
                            uint64_t Pos = HeaderLines[i].find(":", 0);
                            Assert(Pos != std::string::npos, "Failed to find : character in header");

                            std::string Key = HeaderLines[i].substr(0, Pos);
                            std::string Value = HeaderLines[i].substr(Pos + 1);

                            Headers[Key] = Value;
                        }
                    }
                }
            }
        }
    }

    delete[] PFDS;
    return 0;
}
