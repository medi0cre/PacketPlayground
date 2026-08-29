#include <Utils.h>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <Logger.h>

void Enforce(bool Condition, const char* Message)
{
    if (Condition) { return; }
    Logger::Get().Critical(Message);
    std::exit(EXIT_FAILURE);
}

bool IsValidToken(std::string_view Token)
{
    if (Token.size() == 0) { return false; }

    for (size_t i = 0; i < Token.size(); i++)
    {
        char C = Token[i];
        if (!((C >= 'a' && C <= 'z')
        || (C >= 'A' && C <= 'Z')
        || (C >= '0' && C <= '9')
        || C == '!' || C == '#' || C == '$'
        || C == '%' || C == '&' || C == '\''
        || C == '*' || C == '+' || C == '-'
        || C == '.' || C == '^' || C == '_'
        || C == '`' || C == '|' || C == '~')) { return false; }
    }

    return true;
}

bool IsValidHTTPVersion(std::string_view Version)
{
    return Version == "HTTP/1.0" || Version == "HTTP/1.1";
}

bool IsHexDigit(char C)
{
    return ((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F'));
}

long long ParseHex(std::string_view Hex)
{
    if (Hex.size() == 0 || Hex.size() > 8) { return -1; }

    long long Value = 0;
    for (size_t i = 0; i < Hex.size(); i++)
    {
        Value *= 16;
        char C = Hex[i];

        if (C >= '0' && C <= '9') { Value += C - '0'; }
        else if (C >= 'a' && C <= 'f') { Value += (C - 'a' + 10); }
        else if (C >= 'A' && C <= 'F') { Value += (C - 'A' + 10); }
        else { return -1; }
    }

    if (Value > INT_MAX) { return -1; }
    return Value;
}

bool IsValidMethod(std::string_view Method)
{
    return Method == "GET"
        || Method == "POST"
        || Method == "PATCH"
        || Method == "DELETE"
        || Method == "HEAD"
        || Method == "PUT"
        || Method == "CONNECT"
        || Method == "OPTIONS"
        || Method == "TRACE";
}

void TrimTrailingWhiteSpace(std::string_view& String)
{
    while (!String.empty())
    {
        char C = String.back();
        if (C != ' ' && C != '\t' && C != '\r' && C != '\n') { break; }
        String.remove_suffix(1);
    }
}

bool CompareInsensitive(std::string_view View1, std::string_view View2)
{
    if (View1.size() != View2.size()) { return false; }

    for (size_t i = 0; i < View1.size(); i++)
    {
        if (std::tolower(static_cast<unsigned char>(View1[i])) != std::tolower(static_cast<unsigned char>(View2[i]))) { return false; }
    }

    return true;
}
