#include "Client.h"

void Client::Start() 
{
	void* input = nullptr;
	const int argc = 0;
	const char* argv[] = { "" };

	m_socket.ResetMemory();
	m_socket.SetFamily(AF_UNSPEC);
	m_socket.SetSocketType(SOCK_STREAM);

	m_socket.GetAddressInfo("google.com", "80");

	for (addrinfo* p = m_socket.Info(); p != NULL; p = p->ai_next)
	{
		if (!m_socket.Init())
		{
			continue;
		}

		sockaddr* address = reinterpret_cast<sockaddr*>(p->ai_addr);

		if (address->sa_family == AF_INET)
		{
			input = &(reinterpret_cast<sockaddr_in*>(address)->sin_addr);
		}
		else
		{
			input = &(reinterpret_cast<sockaddr_in6*>(address)->sin6_addr);
		}

		inet_ntop(p->ai_family, input, m_socket.IP, sizeof m_socket.IP);
		std::cout << "Client: Attempting to connect to " << m_socket.IP << "\n";

		if (!m_socket.Connect())
		{
			continue;
		}

		break;
	}


	addrinfo* info = m_socket.Info();
	sockaddr* address = reinterpret_cast<sockaddr*>(info->ai_addr);

	if (address->sa_family == AF_INET)
	{
		input = &(reinterpret_cast<sockaddr_in*>(address)->sin_addr);
	}
	else
	{
		input = &(reinterpret_cast<sockaddr_in6*>(address)->sin6_addr);
	}

	inet_ntop(info->ai_family, input, m_socket.IP, sizeof m_socket.IP);
	std::cout << "Client: Connected to " << m_socket.IP << "\n";

	m_socket.FreeAddress();

	// TEST
	const char* request = "GET / HTTP/1.0\r\n\r\n";
	m_socket.Send(request, strlen(request));

	m_socket.Receive(99);
	m_socket.Close();
}