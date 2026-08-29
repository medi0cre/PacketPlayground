#include <Server.h>
#include <ResponseBuilder.h>
#include <Logger.h>
#include <Utils.h>

int main()
{
    Logger::Get().SetLevel(LogLevel::LogInfo);
    PPG::Server* PPG = new PPG::Server("127.0.0.1", "8080");

    PPG->AddRoute("GET", "/", [](const PPG::Request& Req)
    {
        // Just to make my compiler stop complaining about unused variables
        Logger::Get().Trace("Root Method: " + std::string(Req.Method));
        Logger::Get().Trace("Root URI: " + std::string(Req.URI));
        Logger::Get().Trace("Root Version: " + std::string(Req.Version));
        Logger::Get().Trace("Root Path: " + std::string(Req.Path));
        Logger::Get().Trace("Root Query: " + std::string(Req.Query));
        Logger::Get().Trace("Root Body: " + std::string(Req.Body));
        Logger::Get().Trace("Root Content Length: " + std::to_string(Req.ContentLength));

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
