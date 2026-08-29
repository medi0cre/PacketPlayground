#include <Utils.h>
#include <cstring>
#include <cstdlib>
#include <cctype>
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

bool ParseHex(std::string_view Hex, uint64_t& Value)
{
    // Any string bigger than 16 characters cannot be stored as a number, so just return error
    if (Hex.empty() || Hex.size() > 16) { return false; }

    Value = 0;
    for (char C : Hex)
    {
        char Digit = 0;

        if (C >= '0' && C <= '9') { Digit = C - '0'; }
        else if (C >= 'a' && C <= 'f') { Digit = C - 'a' + 10; }
        else if (C >= 'A' && C <= 'F') { Digit = C - 'A' + 10; }
        else { return false; }

        Value = Value * 16 + static_cast<uint64_t>(Digit);
    }

    return true;
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
