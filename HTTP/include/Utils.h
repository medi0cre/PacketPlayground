#pragma once
#include <string>
#include <Parser.h>

void Enforce(bool Condition, const char* Message);
bool IsValidToken(const std::string_view& Token);
bool IsValidURI(const std::string_view& URI);
bool IsValidHTTPVersion(const std::string_view& Version);
bool IsHexDigit(const char C);
long long ParseHex(const std::string_view& Hex);
std::string URLDecode(const std::string_view& Encoded);
bool IsValidMethod(const std::string_view& Method);
void TrimTrailingWhiteSpace(std::string_view& String);
bool CompareInsensitive(const std::string_view& View, const char* String);
