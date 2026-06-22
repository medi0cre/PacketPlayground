#include <iostream>
#include <cstdint>
#include <algorithm>

#include "Utils.h"

void Enforce(bool Condition, const char* Message)
{
    if (Condition)
    {
        return;
    }

    std::cerr << Message << "\n";
    std::abort();
}

std::vector<std::string> SplitByDelimiter(const std::string& Input, const std::string& Delimiter)
{
    std::vector<std::string> Result{};
    uint64_t Start = 0;

    if (Delimiter.empty())
    {
        Result.push_back(Input);
        return Result;
    }

    while (true)
    {
        uint64_t End = Input.find(Delimiter, Start);

        if (End == std::string::npos)
        {
            Result.emplace_back(Input.substr(Start));
            break;
        }

        Result.emplace_back(Input.substr(Start, End - Start));
        Start = End + Delimiter.length();
    }

    return Result;
}

std::string Trim(const std::string& String)
{
    uint64_t First = String.find_first_not_of(" \t\r\n");
    if (First == std::string::npos)
    {
        return "";
    }

    uint64_t Last = String.find_last_not_of(" \t\r\n");
    return String.substr(First, Last - First + 1);
}

std::string LowerCase(std::string Str)
{
    std::transform(Str.begin(), Str.end(), Str.begin(),
    [](unsigned char Character)
    {
        return std::tolower(Character);
    });

    return Str;
}

int SpaceCount(const std::string& Str)
{
    int Count = 0;

    for (int i = 0; i < Str.length(); i++)
    {
        if (Str[i] == ' ')
        {
            Count++;
        }
    }

    return Count;
}

int WhiteSpaceCount(const std::string& Str)
{
    int Count = 0;

    for (int i = 0; i < Str.length(); i++)
    {
        if (Str[i] == ' ' || Str[i] == '\t' || Str[i] == '\r' || Str[i] == '\n')
        {
            Count++;
        }
    }

    return Count;
}
