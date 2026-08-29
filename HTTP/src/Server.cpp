#include <Utils.h>
#include <Server.h>
#include <ResponseBuilder.h>
#include <Logger.h>
#include <climits>

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
            Logger::Get().Warn("Invalid socket, trying the next one...");
            continue;
        }

        char Options = 1;
        Enforce(setsockopt(ListenFD, SOL_SOCKET, SO_REUSEADDR, &Options, sizeof(Options)) == 0, "Failed to set socket options");
        Enforce(Info->ai_addrlen <= INT_MAX, "Underflow detected, not safe to cast to int");

        if (bind(ListenFD, Info->ai_addr, static_cast<int>(Info->ai_addrlen)) == SOCKET_ERROR)
        {
            closesocket(ListenFD);
            Logger::Get().Warn("Failed to bind socket, trying the next one...");
            continue;
        }

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
    Logger::Get().Trace("Listening socket: " + std::to_string(ListenFD));
    Enforce(ConnectionList.size() == 1, "There should not be any other connections other than the listening socket");
}

void PPG::Server::Run()
{
    Logger::Get().Info("Launching server. Waiting for new connections...");

    while (true)
    {
        std::vector<WSAPOLLFD> PollFDList{};
        PollFDList.reserve(ConnectionList.size());

        for (size_t i = 0; i < ConnectionList.size(); i++) { PollFDList.emplace_back(ConnectionList[i].Socket); }

        Enforce(PollFDList.size() == ConnectionList.size(), "Mapping wrong between PollFDList and ConnectionList");
        Enforce(PollFDList.size() <= ULONG_MAX, "Not safe to cast to ULONG");
        Enforce(WSAPoll(PollFDList.data(), static_cast<ULONG>(PollFDList.size()), MaxTimeout) != SOCKET_ERROR, "Error occured during polling");

        for (size_t ConnectionIndex = ConnectionList.size(); ConnectionIndex-- > 0; )
        {
            if (ConnectionIndex != 0)
            {
                const Time Now = Clock::now();
                const Time LastEvent = ConnectionList[ConnectionIndex].LastEvent;
                const size_t IdleDuration = static_cast<size_t>(std::chrono::duration_cast<MS>(Now - LastEvent).count());

                if (IdleDuration >= MaxTimeout)
                {
                    Logger::Get().Info("Timeout on socket " + std::to_string(ConnectionList[ConnectionIndex].Socket.fd));
                    RemoveConnection(ConnectionIndex);
                    continue;
                }
            }

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
        Logger::Get().Error("Invalid file descriptor obtained from accept() call");
        Logger::Get().Error("Error code: " + std::to_string(WSAGetLastError()));
        return;
    }

    if (ConnectionList.size() >= MaxConnections)
    {
        Logger::Get().Error("Reached maximum connection limit, rejecting socket " + std::to_string(NewFD));
        closesocket(NewFD);
        return;
    }

    // Add the new socket to the poll
    Connection Client{};
    Client.Socket.fd = NewFD;
    Client.Socket.events = POLLIN;
    Client.Socket.revents = 0;
    Client.Par.State = ParseState::RequestLineStart;
    Client.LastEvent = Clock::now();
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

    Logger::Get().Info("New connection from " + std::string(RemoteIP) + " on socket " + std::to_string(NewFD));
}

void PPG::Server::HandleClientData(size_t Index)
{
    Enforce(Index < ConnectionList.size() && Index < MaxConnections, "Invalid connection index");

    char Data[ReceiveBufferSize];
    int NumBytes = recv(ConnectionList[Index].Socket.fd, Data, ReceiveBufferSize, 0);
    Logger::Get().Trace("Received " + std::to_string(NumBytes) + " bytes");

    if (NumBytes <= 0)
    {
        if (NumBytes == 0) { Logger::Get().Info("Socket " + std::to_string(ConnectionList[Index].Socket.fd) + " closed the connection"); }
        else
        {
            Logger::Get().Error("Error receiving message from socket "
                + std::to_string(ConnectionList[Index].Socket.fd)
                + "\nError Code: " + std::to_string(WSAGetLastError()));
        }

        RemoveConnection(Index);
        return;
    }

    ConnectionList[Index].LastEvent = Clock::now();
    ConnectionList[Index].Par.Buffer.append(Data, static_cast<size_t>(NumBytes));

    if (ConnectionList[Index].Par.Buffer.size() > MaxBufferSize)
    {
        // DDoS attack!
        Logger::Get().Error("Exceeded buffer limit on socket " + std::to_string(ConnectionList[Index].Socket.fd));
        RemoveConnection(Index);
        return;
    }

    while (true)
    {
        int Result = ConnectionList[Index].Par.Parse();
        Logger::Get().Info("Parsing result in main loop: " + std::to_string(Result));

        switch (Result)
        {
        case -1:
        {
            // Error: Send bad request and dip
            Logger::Get().Error("Bad request on socket " + std::to_string(ConnectionList[Index].Socket.fd));
            ResponseBuilder Builder{};
            Response Res = Builder.BadRequest().Build();
            SendResponse(ConnectionList[Index].Socket.fd, Res);
            RemoveConnection(Index);
            return;
        }
        case 0:
        {
            // Need more data!
            Logger::Get().Trace("Incomplete request. Returning and waiting for new data ...");
            return;
        }
        case 1:
        {
            Parser& Par = ConnectionList[Index].Par;
            ProcessRequest(Index, Par.Req);
            Logger::Get().Trace("Finished processing request");

            bool CloseFlag = false;
            bool KeepAliveFlag = false;

            for (size_t i = 0; i < Par.Req.Headers.size(); i++)
            {
                if (Par.Req.Headers[i].Key != "connection") { continue; }
                if (Par.Req.Headers[i].Value.find("close") != std::string::npos) { CloseFlag = true; }
                if (Par.Req.Headers[i].Value.find("keep-alive") != std::string::npos) { KeepAliveFlag = true; }
            }

            if (CloseFlag)
            {
                Logger::Get().Trace("Close flag located");
                RemoveConnection(Index);
                return;
            }
            else if (KeepAliveFlag)
            {
                Logger::Get().Trace("Keep alive flag located");
                Enforce(Par.Position <= Par.Buffer.size(), "Parser position exceeds parser buffer");
                Par.Reset();

                if (Par.Position >= Par.Buffer.size())
                {
                    Logger::Get().Trace("Reached the end of buffer. Waiting for more data or 0 signal ...");
                    return; 
                }
            }
            else if (Par.Req.Version == "HTTP/1.1")
            {
                Logger::Get().Trace("No flag located in HTTP 1.1 request. Defaulting to keep alive");
                Enforce(Par.Position <= Par.Buffer.size(), "Parser position exceeds parser buffer");
                Par.Reset();

                if (Par.Position >= Par.Buffer.size())
                {
                    Logger::Get().Trace("Reached the end of buffer. Waiting for more data or 0 signal ...");
                    return; 
                }
            }
            else if (Par.Req.Version == "HTTP/1.0")
            {
                Logger::Get().Trace("No flag located in HTTP 1.0 request. Defaulting to close");
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
    Enforce(Res.StatusCode != 0 && Res.Status != "" && Res.Version != "", "Default response should never be sent to any user ever");

    std::string Result = "";
    Result.append(Res.Version);
    Result.append(" ");
    Result.append(std::to_string(static_cast<unsigned>(Res.StatusCode)));
    Result.append(" ");
    Result.append(Res.Status);
    Result.append("\r\n");

    for (const auto& Header: Res.Headers)
    {
        Result.append(Header.Key);
        Result.append(": ");
        Result.append(Header.Value);
        Result.append("\r\n");
    }

    Result.append("\r\n");
    Result.append(Res.Body);
    Logger::Get().Trace("Response to send:\n" + Result);

    return Result;
}

void PPG::Server::ProcessRequest(size_t Index, const Request& Req)
{
    Enforce(Index > 0 && Index < ConnectionList.size(), "Invalid connection index");
    Enforce(Req.Method != "" && Req.Path != "", "Invalid method or path");

    std::string Key = "";
    Key.append(Req.Method);
    Key.append(":");
    Key.append(Req.Path);
    Logger::Get().Trace("Route key: " + Key);

    auto It = Routes.find(Key);
    if (It == Routes.end())
    {
        Logger::Get().Warn("No route found (404 error) on socket " + std::to_string(ConnectionList[Index].Socket.fd));
        ResponseBuilder Builder{};
        SendResponse(ConnectionList[Index].Socket.fd, Builder.NotFound().Build());
        return;
    }

    Logger::Get().Info("Route found on socket " + std::to_string(ConnectionList[Index].Socket.fd));
    SendResponse(ConnectionList[Index].Socket.fd, It->second(Req));
}

void PPG::Server::SendResponse(SOCKET ClientFD, const Response& Res)
{
    const std::string ResponseString = Stringify(Res);
    Enforce(ResponseString.size() != 0 && ResponseString.size() <= INT_MAX, "Response should NEVER be empty or larger than int max");

    size_t TotalSent = 0;
    size_t Remaining = ResponseString.size();
    Logger::Get().Trace("Response size: " + std::to_string(Remaining));

    while (Remaining > 0)
    {
        int Sent = send(ClientFD, ResponseString.c_str() + TotalSent, static_cast<int>(Remaining), 0);
        if (Sent == SOCKET_ERROR)
        {
            Logger::Get().Error("Failed to send response: " + std::to_string(WSAGetLastError()));
            return;
        }

        Logger::Get().Trace("Sent " + std::to_string(Sent) + " bytes");

        TotalSent += static_cast<size_t>(Sent);
        Remaining -= static_cast<size_t>(Sent);
    }

    Enforce(Remaining == 0 && TotalSent == ResponseString.size(), "Response should always be fully sent unless there is a bug");
}

void PPG::Server::RemoveConnection(size_t Index)
{
    Enforce(Index > 0 && Index < ConnectionList.size(), "Invariant broken inside RemoveConnections()");
    Logger::Get().Info("Removing socket " + std::to_string(ConnectionList[Index].Socket.fd));
    if (ConnectionList[Index].Socket.fd != INVALID_SOCKET) { closesocket(ConnectionList[Index].Socket.fd); }
    ConnectionList.erase(ConnectionList.begin() + static_cast<long long>(Index));
}

void PPG::Server::AddRoute(std::string_view Method, std::string_view Path, std::function<Response(const Request&)> Dispatcher)
{
    if (Path.size() == 0)
    {
        Logger::Get().Error("Empty path provided, failed to add route");
        return;
    }

    if (!IsValidMethod(Method))
    {
        Logger::Get().Error("Invalid method provided, failed to add route");
        Logger::Get().Error("Please check whether all methods are uppercase and valid, e.g. GET, POST, PATCH");
        return;
    }

    std::string Key = "";
    Key.append(Method);
    Key.append(":");
    Key.append(Path);

    Routes[Key] = Dispatcher;
    Logger::Get().Info("Added route: " + Key);
}

PPG::Server::~Server()
{
    for (size_t i = 0; i < ConnectionList.size(); i++)
    {
        if (ConnectionList[i].Socket.fd != INVALID_SOCKET) { closesocket(ConnectionList[i].Socket.fd); }
    }

    Logger::Get().Info("Shutting down server");
    WSACleanup();
}
