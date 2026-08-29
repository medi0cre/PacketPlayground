#include <Utils.h>
#include <cstring>
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

// This function still needs a lot of work
bool IsValidURI(std::string_view URI)
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

bool IsValidHTTPVersion(std::string_view Version)
{
    return Version == "HTTP/1.0" || Version == "HTTP/1.1";
}

bool IsHexDigit(const char C)
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

std::string URLDecode(std::string_view Encoded)
{
    std::string Result = "";
    size_t i = 0;

    while (i < Encoded.size())
    {
        if (Encoded[i] == '%')
        {
            if (i + 2 < Encoded.size())
            {
                if (IsHexDigit(Encoded[i + 1]) && IsHexDigit(Encoded[i + 2]))
                {
                    long long ByteValue = ParseHex(Encoded.substr(i + 1, 2));
                    if (ByteValue < 0 || ByteValue > UCHAR_MAX) { return ""; }

                    Result.push_back(static_cast<char>(ByteValue));
                    i += 3;
                    continue;
                }
            }
        }

        Result.push_back(Encoded[i]);
        i++;
    }

    return Result;
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

bool CompareInsensitive(std::string_view View, const char* String)
{
    if (View.size() != strlen(String)) { return false; }

    for (size_t i = 0; i < View.size(); i++)
    {
        if (std::tolower(View[i]) != std::tolower(String[i])) { return false; }
    }

    return true;
}
