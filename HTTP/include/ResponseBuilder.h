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
        ResponseBuilder& Version(std::string_view Version);
        ResponseBuilder& StatusCode(uint16_t StatusCode);
        ResponseBuilder& Status(std::string_view Status);
        ResponseBuilder& Body(std::string_view Body);
        ResponseBuilder& Header(std::string_view Key, std::string_view Value);

        ResponseBuilder& NotFound();
        ResponseBuilder& BadRequest();

        ResponseBuilder& OK();
        ResponseBuilder& JSON();
        ResponseBuilder& HTML();
    };
}
