#include <Server.h>
#include <Test.h>
#include <ResponseBuilder.h>

int main()
{
    RunTests();
    HTTP::Server* PPG = new HTTP::Server("127.0.0.1", "8080");

    PPG->AddRoute("GET", "/", [](const HTTP::Request& Req)
    {
        HTTP::ResponseBuilder Builder;
        return Builder
            .OK()
            .HTML()
            .Body("<h1>Hello World</h1>")
            .Build();
    });

    PPG->AddRoute("POST", "/echo", [](const HTTP::Request& Req)
    {
        HTTP::ResponseBuilder Builder;
        return Builder
            .OK()
            .JSON()
            .Body(Req.Body)
            .Build();
    });

    PPG->Run();
    return 0;
}
