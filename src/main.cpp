#include "PacketPlayground.h"

int main()
{
	PacketPlayground ppg;
	ppg.Init();
	ppg.StartServer();

	return 0;
}