#pragma once

#include "Socket.h"

class Client 
{
private:
	Socket m_socket;

public:
	void Start();
};