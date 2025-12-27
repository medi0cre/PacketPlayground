#pragma once

#define WIN32_LEAN_AND_MEAN  

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

class Socket 
{
private:
	int m_fileDesc = 0;  
	int m_acceptDesc = 0;
	int m_connectionCount = 0;
	int m_addressSize = 0;
	struct addrinfo m_hints = {};
	struct addrinfo* m_info = nullptr;
	struct addrinfo* m_pointer = nullptr;
	struct sockaddr_storage m_storage = {};
	char* m_buffer = nullptr;
	char m_ip[INET6_ADDRSTRLEN] = "";

public:
	void ResetMemory();
	void SetFamily(const int family);
	void SetSocketType(const int type);
	void SetConnectionCount(const int count);
	void SetFlag(const int flag);
	void GetAddressInfo(const std::string& URL, const std::string& port);
	void Init();
	void Connect();
	void Listen();
	void Accept();
	void Send(const char* message, const int length);
	void Receive(const int length);
	void Close();
	void FreeAddress();
};