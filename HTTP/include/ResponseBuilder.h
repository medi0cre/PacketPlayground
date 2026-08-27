#pragma once
#include <Server.h>
#include <string_view>

namespace PPG
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
        ResponseBuilder& Version(const std::string_view& Version);
        ResponseBuilder& StatusCode(uint16_t StatusCode);
        ResponseBuilder& Status(const std::string_view& Status);
        ResponseBuilder& Body(const std::string_view& Body);
        ResponseBuilder& Header(const std::string_view& Key, const std::string_view& Value);

        ResponseBuilder& NotFound();
        ResponseBuilder& BadRequest();

        ResponseBuilder& OK();
        ResponseBuilder& JSON();
        ResponseBuilder& HTML();
    };
}
