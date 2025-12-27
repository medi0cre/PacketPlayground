#pragma once

#include "Socket.h"

class Server 
{
private:
	Socket m_socket;

public:
	void Start();
};