#include "PacketPlayground.h"
#include "Socket.h"

int main() 
{
	Socket socket;
	PacketPlayground ppg(socket);
	ppg.Init();

	return 0;
}