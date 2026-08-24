#include <Server.h>
#include <ResponseBuilder.h>
#include<Logger.h>

int main()
{
    Logger::Get().SetLevel(LogLevel::LogInfo);
    PPG::Server* PPG = new PPG::Server("127.0.0.1", "8080");

    PPG->AddRoute("GET", "/", [](const PPG::Request& Req)
    {
        PPG::ResponseBuilder Builder;
        return Builder
            .OK()
            .HTML()
            .Body("<h1>Hello World</h1>")
            .Build();
    });

    PPG->AddRoute("POST", "/echo", [](const PPG::Request& Req)
    {
        PPG::ResponseBuilder Builder;
        return Builder
            .OK()
            .JSON()
            .Body(Req.Body)
            .Build();
    });

    PPG->Run();
    return 0;
}
