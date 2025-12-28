#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

class Socket 
{
private:
	int m_fileDesc = 0;  
	int m_acceptDesc = 0;
	int m_connectionCount = 0;
	int m_addressSize = 0;
	addrinfo m_hints = {};
	addrinfo* m_info = nullptr;
	addrinfo* m_pointer = nullptr;
	sockaddr_storage m_storage = {};
	char* m_buffer = nullptr;

public:
	char IP[INET6_ADDRSTRLEN] = "";

	void ResetMemory();
	void SetFamily(const int family);
	void SetSocketType(const int type);
	void SetConnectionCount(const int count);
	void SetFlag(const int flag);
	void GetAddressInfo(const char* URL, const char* port);
	void Init();
	void Connect();
	void Listen();
	void Accept();
	void Send(const char* message, const int length);
	void Receive(const int length);
	void Close();
	void FreeAddress();

	addrinfo* Info();
	const int FileDesc();
	sockaddr_storage& Storage();
};