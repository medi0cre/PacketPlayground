#pragma once
#include <string>

void Enforce(bool Condition, const char* Message);
std::string ToLower(std::string Str);
bool IsValidToken(const std::string& Token);
bool IsValidURI(const std::string& URI);
bool IsValidHTTPVersion(const std::string& Version);
bool IsHexDigit(const char C);
long long ParseHex(const std::string& Hex);
std::string URLDecode(const std::string& Encoded);
bool IsValidMethod(const std::string& Method);
std::string TrimTrailingWhiteSpace(const std::string& String);
