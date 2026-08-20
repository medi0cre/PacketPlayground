#pragma once
#include <Server.h>

namespace HTTP
{
    class ResponseBuilder
    {
    private:
        Response Res{};

    public:
        ResponseBuilder() = default;
        ~ResponseBuilder() = default;

        Response Build();
        ResponseBuilder& Reset();
        ResponseBuilder& Version(const std::string& Version);
        ResponseBuilder& StatusCode(uint16_t StatusCode);
        ResponseBuilder& Status(const std::string& Status);
        ResponseBuilder& Body(const std::string& Body);
        ResponseBuilder& Header(const std::string& Key, const std::string& Value);

        ResponseBuilder& NotFound();
        ResponseBuilder& BadRequest();

        ResponseBuilder& OK();
        ResponseBuilder& JSON();
        ResponseBuilder& HTML();
    };
}
