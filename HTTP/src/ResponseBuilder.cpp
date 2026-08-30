#include <ResponseBuilder.h>

PPG::Response PPG::ResponseBuilder::Build()
{
    Res.Headers.emplace_back(StringKV { "Server", "PacketPlayground" });
    Res.Headers.emplace_back(StringKV { "Content-Length", std::to_string(Res.Body.size()) });

    Response Result = Res;
    Res = {};
    return Result;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::Reset()
{
    Res = {};
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::Version(std::string_view Version)
{
    Res.Version = Version;
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::StatusCode(uint16_t StatusCode)
{
    Res.StatusCode = StatusCode;
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::Status(std::string_view Status)
{
    Res.Status = Status;
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::Body(std::string_view Body)
{
    Res.Body = Body;
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::Header(std::string_view Key, std::string_view Value)
{
    Res.Headers.emplace_back(StringKV { std::string(Key), std::string(Value) });
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::NotFound()
{
    const std::string_view NotFoundBody = "<html>"
                                       "<head><title>404 Not Found</title></head>"
                                       "<body>"
                                         "<h1>404 Not Found</h1>"
                                         "<p>The requested resource was not found on this server.</p>"
                                       "</body>"
                                     "</html>";


    Reset();
    Version("HTTP/1.1");
    StatusCode(404);
    Status("Not Found");
    HTML();
    Header("Connection", "close");
    Body(NotFoundBody);

    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::BadRequest()
{
    const std::string_view BadRequestBody = "<html>"
                                         "<head><title>400 Bad Request</title></head>"
                                         "<body>"
                                           "<h1>400 Bad Request</h1>"
                                           "<p>Your browser sent a request that the server could not understand</p>"
                                         "</body>"
                                       "</html>";


    Reset();
    Version("HTTP/1.1");
    StatusCode(400);
    Status("Bad Request");
    HTML();
    Header("Connection", "close");
    Body(BadRequestBody);

    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::OK()
{
    Reset();
    Version("HTTP/1.1");
    StatusCode(200);
    Status("OK");

    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::JSON()
{
    Header("Content-Type", "application/json");
    return *this;
}

PPG::ResponseBuilder& PPG::ResponseBuilder::HTML()
{
    Header("Content-Type", "text/html");
    return *this;
}
