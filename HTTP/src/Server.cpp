#include <iostream>
#include <Utils.h>
#include <Server.h>
#include <ResponseBuilder.h>

PPG::Server::Server(const char* IPAddress, const char* Port)
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

void PPG::Server::Run()
{
    std::cout << "Launching server. Waiting for new connections...\n";

    while (true)
    {
        std::vector<WSAPOLLFD> PollFDList{};
        PollFDList.reserve(ConnectionList.size());

        for (size_t i = 0; i < ConnectionList.size(); i++) { PollFDList.emplace_back(ConnectionList[i].Socket); }

        // Start polling with -1 timeout to poll forever
        Enforce(PollFDList.size() == ConnectionList.size(), "Mapping wrong between PollFDList and ConnectionList");
        Enforce(WSAPoll(PollFDList.data(), PollFDList.size(), -1) != SOCKET_ERROR, "Error occured during polling");

        for (size_t ConnectionIndex = ConnectionList.size(); ConnectionIndex-- > 0; )
        {
            if (PollFDList[ConnectionIndex].revents & (POLLIN | POLLHUP))
            {
                if (ConnectionIndex == 0) { HandleNewConnection(); }
                else { HandleClientData(ConnectionIndex); }
            }
        }
    }
}

void PPG::Server::HandleNewConnection()
{
    sockaddr_storage RemoteAddr{};
    socklen_t AddrLen = sizeof(RemoteAddr);
    SOCKET NewFD = INVALID_SOCKET;
    char RemoteIP[INET6_ADDRSTRLEN];

    NewFD = accept(ConnectionList[0].Socket.fd, reinterpret_cast<sockaddr*>(&RemoteAddr), &AddrLen);
    if (NewFD == INVALID_SOCKET)
    {
        std::cerr << "Invalid file descriptor obtained from accept() call\n";
        closesocket(NewFD);
        return;
    }

    if (ConnectionList.size() >= MaxConnections)
    {
        std::cerr << "No room in poll buffer to add new connection\n";
        closesocket(NewFD);
        return;
    }

    // Add the new socket to the poll
    Connection Client{};
    Client.Socket.fd = NewFD;
    Client.Socket.events = POLLIN;
    Client.Socket.revents = 0;
    Client.Par.State = ParseState::RequestLineStart;
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

void PPG::Server::HandleClientData(size_t Index)
{
    Enforce(Index < ConnectionList.size() && Index < MaxConnections, "Invalid connection index");

    char Data[ReceiveBufferSize];
    int NumBytes = recv(ConnectionList[Index].Socket.fd, Data, ReceiveBufferSize, 0);

    if (NumBytes <= 0)
    {
        if (NumBytes == 0) { std::cerr << "Socket " << ConnectionList[Index].Socket.fd << " closed the connection\n"; }
        else
        {
            std::cerr << "Error receiving message from socket " << ConnectionList[Index].Socket.fd << "\n";
            std::cerr << "Error Code: " << WSAGetLastError() << "\n";
        }

        RemoveConnection(Index);
        return;
    }

    ConnectionList[Index].Par.Buffer += std::string(Data, NumBytes);
    if (ConnectionList[Index].Par.Buffer.size() > MaxBufferSize)
    {
        // DDoS attack!
        RemoveConnection(Index);
        return;
    }

    while (true)
    {
        int Result = ConnectionList[Index].Par.Parse();

        switch (Result)
        {
        case -1:
        {
            // Error: Send bad request and dip
            std::cerr << "Bad request on socket " << ConnectionList[Index].Socket.fd << "\n";
            ResponseBuilder Builder{};
            Response Res = Builder.BadRequest().Build();
            SendResponse(ConnectionList[Index].Socket.fd, Res);
            RemoveConnection(Index);
            return;
        }
        case 0:
        {
            // Need more data!
            return;
        }
        case 1:
        {
            Parser& Par = ConnectionList[Index].Par;
            ProcessRequest(Index, Par.Req);

            bool CloseFlag = false;
            bool KeepAliveFlag = false;

            for (size_t i = 0; i < Par.Req.Headers.size(); i++)
            {
                if (Par.Req.Headers[i].Key != "connection") { continue; }
                std::string Value = ToLower(Par.Req.Headers[i].Value);

                if (Value.find("close") != std::string::npos) { CloseFlag = true; }
                if (Value.find("keep-alive") != std::string::npos) { KeepAliveFlag = true; }
            }

            if (CloseFlag)
            {
                RemoveConnection(Index);
                return;
            }
            else if (KeepAliveFlag)
            {
                Enforce(Par.Position <= Par.Buffer.size(), "Parser position exceeds parser buffer");
                const std::string Remaining = Par.Buffer.substr(Par.Position);
                Par = {};
                Par.Buffer = Remaining;
            }
            else if (Par.Req.Version == "HTTP/1.1")
            {
                Enforce(Par.Position <= Par.Buffer.size(), "Parser position exceeds parser buffer");
                const std::string Remaining = Par.Buffer.substr(Par.Position);
                Par = {};
                Par.Buffer = Remaining;
            }
            else if (Par.Req.Version == "HTTP/1.0")
            {
                RemoveConnection(Index);
                return;
            }
            else { Enforce(false, "Impossible state reached during connection closing decision"); }
            break;
        }
        default:
            Enforce(false, "Invariant broken inside HandleClientData()");
        }
    }
}

std::string PPG::Server::Stringify(const Response& Res)
{
    Enforce(Res.StatusCode != 0 && Res.Status != "" && Res.Body != "" && Res.Version != "",
        "Default response should never be sent to any user ever");

    std::string Result = "";
    Result += Res.Version + " " + std::to_string(Res.StatusCode) + " " + Res.Status + "\r\n";

    for (const auto& Header: Res.Headers) { Result += Header.Key + ": " + Header.Value + "\r\n"; }

    Result += "\r\n" + Res.Body;
    return Result;
}

void PPG::Server::ProcessRequest(size_t Index, const Request& Req)
{
    Enforce(Index > 0 && Index < ConnectionList.size(), "Invalid connection index");
    Enforce(Req.Method != "" && Req.Path != "", "Invalid method or path");

    const std::string Key = Req.Method + ":" + Req.Path;
    auto It = Routes.find(Key);

    if (It == Routes.end())
    {
        ResponseBuilder Builder{};
        SendResponse(ConnectionList[Index].Socket.fd, Builder.NotFound().Build());
        return;
    }

    SendResponse(ConnectionList[Index].Socket.fd, It->second(Req));
}

void PPG::Server::SendResponse(SOCKET ClientFD, const Response& Res)
{
    std::string ResponseString = Stringify(Res);
    Enforce(ResponseString.size() != 0, "Response should NEVER be empty");

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

    Enforce(Remaining == 0 && TotalSent == ResponseString.size(), "Response should always be fully sent unless there is a bug");
}

void PPG::Server::RemoveConnection(size_t Index)
{
    Enforce(Index > 0 && Index < MaxConnections, "Invariant broken inside RemoveConnections()");
    closesocket(ConnectionList[Index].Socket.fd);
    ConnectionList.erase(ConnectionList.begin() + Index);
}

void PPG::Server::AddRoute(std::string Method, const std::string& Path, std::function<Response(const Request&)> Dispatcher)
{
    if (Path.size() == 0)
    {
        std::cerr << "Empty path provided, failed to add route\n";
        return;
    }

    if (!IsValidMethod(Method))
    {
        std::cerr << "Invalid method provided, failed to add route\n";
        std::cerr << "Please check whether all methods are uppercase and valid, e.g. GET, POST, PATCH\n";
        return;
    }

    std::string Key = Method + ":" + Path;
    Routes[Key] = Dispatcher;
    std::cout << "Added route: " << Method << " " << Path << "\n";
}

PPG::Server::~Server()
{
    for (size_t i = 0; i < ConnectionList.size(); i++)
    {
        if (ConnectionList[i].Socket.fd != INVALID_SOCKET) { closesocket(ConnectionList[i].Socket.fd); }
    }

    WSACleanup();
}
