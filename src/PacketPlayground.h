#pragma once

#define WIN32_LEAN_AND_MEAN  

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#include "Socket.h"

class PacketPlayground 
{
private:
	Socket m_socket;

public:
	PacketPlayground(const Socket& socket);
	~PacketPlayground();
	void Init();
};