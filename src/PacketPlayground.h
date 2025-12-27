#pragma once

#include "Server.h"
#include "Client.h"

class PacketPlayground 
{
private:
	Server m_server;
	Client m_client;

public:
	void Init();
	void StartServer();
};