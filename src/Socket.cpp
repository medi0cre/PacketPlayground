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

bool Socket::GetAddressInfo(const char* URL, const char* port) 
{
	if (getaddrinfo(URL, port, &m_hints, &m_info) != 0)
	{
		std::cerr << "Error obtaining address info\n";
		return false;
	}

	return true;
}

void Socket::SetConnectionCount(const int count) 
{
	m_connectionCount = count;
}

bool Socket::Init() 
{
	m_fileDesc = static_cast<int>(socket(m_info->ai_family, m_info->ai_socktype, m_info->ai_protocol));

	if (m_fileDesc == -1) 
	{
		std::cerr << "Error obtaining socket file descriptor\n";
		return false;
	}
	
	// Client does not need port binding 
	// Uncomment for server ports
	// if (bind(m_fileDesc, m_info->ai_addr, static_cast<int>(m_info->ai_addrlen)) == -1) 
	// {
	//	   std::cerr << "Error binding socket\n";
	//	   Close();
	//	   return false;
	// }
	
	return true;
}

bool Socket::Connect() 
{
	if (connect(m_fileDesc, m_info->ai_addr, static_cast<int>(m_info->ai_addrlen)) == -1) 
	{
		std::cerr << "Connection error\n";
		return false;
	}

	return true;
}

bool Socket::Listen()
{
	if (listen(m_fileDesc, m_connectionCount) == -1) 
	{
		std::cerr << "Socket listening error\n";
		return false;
	}
}

bool Socket::Accept() 
{
	m_addressSize = sizeof(m_storage);
	m_acceptDesc = accept(m_fileDesc, reinterpret_cast<sockaddr*>(&m_storage), &m_addressSize);

	if (m_acceptDesc == -1) 
	{
		std::cerr << "Error accepting socket connection\n";
		return false;
	}

	return true;
}

bool Socket::Send(const char* message, const int length)
{
	if (send(m_fileDesc, message, length, 0) == -1) 
	{
		std::cerr << "Error sending data\n";
		return false;
	}

	return true;
}

bool Socket::Receive(const int length)
{
	int bytes;

	if ((bytes = recv(m_fileDesc, m_buffer, length, 0)) == -1)
	{
		std::cerr << "Error receiving data\n";
		return false;
	}

	m_buffer[bytes] = '\0';
	std::cout << "Received " << m_buffer << "\n";

	return true;
}

void Socket::Close() 
{
	closesocket(m_fileDesc);
}

void Socket::FreeAddress() 
{
	freeaddrinfo(m_info);
	m_info = nullptr;
}

addrinfo* Socket::Info()
{
	return m_info;
}

const int Socket::FileDesc() 
{
	return m_fileDesc;
}

sockaddr_storage& Socket::Storage() 
{
	return m_storage;
}

char* Socket::Buffer() 
{
	return m_buffer;
}