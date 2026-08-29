#pragma once
#include <string>
#include <string_view>

void Enforce(bool Condition, const char* Message);
bool IsValidToken(std::string_view Token);
bool IsValidURI(std::string_view URI);
bool IsValidHTTPVersion(std::string_view Version);
bool IsHexDigit(char C);
long long ParseHex(std::string_view Hex);
std::string URLDecode(std::string_view Encoded);
bool IsValidMethod(std::string_view Method);
void TrimTrailingWhiteSpace(std::string_view& String);
bool CompareInsensitive(std::string_view View1, std::string_view View2);
