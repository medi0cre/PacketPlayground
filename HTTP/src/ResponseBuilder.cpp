#include <ResponseBuilder.h>

HTTP::Response HTTP::ResponseBuilder::Build()
{
    Res.Headers.emplace_back(KV { "Server", "PacketPlayground" });
    Res.Headers.emplace_back(KV { "Content-Length", std::to_string(Res.Body.size()) });
    return Res;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Reset()
{
    Res = {};
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Version(const std::string& Version)
{
    Res.Version = Version;
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::StatusCode(uint16_t StatusCode)
{
    Res.StatusCode = StatusCode;
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Status(const std::string& Status)
{
    Res.Status = Status;
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Body(const std::string& Body)
{
    Res.Body = Body;
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::Header(const std::string& Key, const std::string& Value)
{
    Res.Headers.emplace_back(KV { Key, Value });
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::NotFound()
{
    const std::string NotFoundBody = "<html>"
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

HTTP::ResponseBuilder& HTTP::ResponseBuilder::BadRequest()
{
    const std::string BadRequestBody = "<html>"
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

HTTP::ResponseBuilder& HTTP::ResponseBuilder::OK()
{
    Reset();
    Version("HTTP/1.1");
    StatusCode(200);
    Status("OK");

    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::JSON()
{
    Header("Content-Type", "application/json");
    return *this;
}

HTTP::ResponseBuilder& HTTP::ResponseBuilder::HTML()
{
    Header("Content-Type", "text/html");
    return *this;
}
