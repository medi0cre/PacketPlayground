#pragma once
#include <string_view>

void Enforce(bool Condition, const char* Message);
bool IsValidToken(std::string_view Token);
bool IsValidHTTPVersion(std::string_view Version);
bool IsHexDigit(char C);
long long ParseHex(std::string_view Hex);
bool IsValidMethod(std::string_view Method);
void TrimTrailingWhiteSpace(std::string_view& String);
bool CompareInsensitive(std::string_view View1, std::string_view View2);
