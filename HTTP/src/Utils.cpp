#include <iostream>
#include <algorithm>

#include <Utils.h>

void Enforce(bool Condition, const char* Message)
{
    if (Condition) { return; }
    std::cerr << Message << "\n";
    std::exit(EXIT_FAILURE);
}

std::string ToLower(std::string Str)
{
    std::transform(Str.begin(), Str.end(), Str.begin(),
    [](unsigned char Character)
    {
        return std::tolower(Character);
    });

    return Str;
}

bool IsValidToken(const std::string& Token)
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

// This function still needs a lot of work
bool IsValidURI(const std::string& URI)
{
    if (URI.size() == 0) { return false; }

    for (size_t i = 0; i < URI.size(); i++)
    {
        char C = URI[i];
        if (C <= 0x20 || C >= 0x7F) { return false; }
        if (C == '%')
        {
            if (i + 2 >= URI.size()) { return false; }
            if (!IsHexDigit(URI[i + 1]) || !IsHexDigit(URI[i + 2])) { return false; }
            i += 2;
        }
    }

    return true;
}

bool IsValidHTTPVersion(const std::string& Version)
{
    return (Version == "HTTP/1.0" || Version == "HTTP/1.1");
}

bool IsHexDigit(const char C)
{
    return ((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F'));
}

long long ParseHex(const std::string& Hex)
{
    if (Hex.size() == 0) { return -1; }

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

    if (Value > 0x7FFFFFFF) { return -1; }
    return Value;
}

// Minor improvements needed
std::string URLDecode(const std::string& Encoded)
{
    std::string Result = "";
    size_t i = 0;

    while (i < Encoded.size())
    {
        if (Encoded[i] == '%')
        {
            if (i + 2 < Encoded.size())
            {
                std::string HexByte = Encoded.substr(i + 1, 2);
                if (IsHexDigit(Encoded[i + 1]) && IsHexDigit(Encoded[i + 2]))
                {
                    int ByteValue = ParseHex(HexByte);
                    Result += std::to_string(ByteValue);
                    i += 3;
                    continue;
                }
            }
        }

        Result += Encoded[i];
        i++;
    }

    return Result;
}

bool IsValidMethod(const std::string& Method)
{
    return Method == "GET"
        || Method == "POST"
        || Method == "PATCH"
        || Method == "DELETE"
        || Method == "HEAD"
        || Method == "PUT"
        || Method == "CONNECT"
        || Method == "OPTIONS"
        || Method == "TRACE"
        || Method == "PATCH";
}
