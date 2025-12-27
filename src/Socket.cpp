#include "Socket.h"

void Socket::ResetMemory()
{
	memset(&m_hints, 0, sizeof m_hints);
}

void Socket::SetFamily(const int family)
{
	m_hints.ai_family = family;
}

void Socket::SetSocketType(const int type) 
{
	m_hints.ai_socktype = type;
}

void Socket::SetFlag(const int flag) 
{
	m_hints.ai_flags = flag;
}

void Socket::GetAddressInfo(const std::string& URL, const std::string& port) 
{
	if (getaddrinfo(URL.c_str(), port.c_str(), &m_hints, &m_info) == 0)
	{
		std::cout << "Acquired address info\n";
	}
	else 
	{
		std::cerr << "Error obtaining address info\n";
	}
}

void Socket::SetConnectionCount(const int count) 
{
	m_connectionCount = count;
}

void Socket::Init() 
{
	m_fileDesc = static_cast<int>(socket(m_info->ai_family, m_info->ai_socktype, m_info->ai_protocol));
	bind(m_fileDesc, m_info->ai_addr, static_cast<int>(m_info->ai_addrlen));
}

void Socket::Connect() 
{
	connect(m_fileDesc, m_info->ai_addr, static_cast<int>(m_info->ai_addrlen));
}

void Socket::Listen()
{
	listen(m_fileDesc, m_connectionCount);
}

void Socket::Accept() 
{
	m_addressSize = sizeof(m_storage);
	m_acceptDesc = accept(m_fileDesc, (struct sockaddr*)&m_storage, &m_addressSize);
}

void Socket::Send(const char* message, const int length)
{
	send(m_fileDesc, message, length, 0);
}

void Socket::Receive(const int length)
{
	recv(m_fileDesc, m_buffer, length, 0);
}

void Socket::Close() 
{
	closesocket(m_fileDesc);
}

void Socket::FreeAddress() 
{
	freeaddrinfo(m_info);
}