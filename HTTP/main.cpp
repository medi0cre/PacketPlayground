#include "Server.h"

int main()
{
    HTTP::Server* MyServer = new HTTP::Server("127.0.0.1", "8080");
    MyServer->Run();
    delete MyServer;
    return 0;
}
