#pragma once
#include <vector>
#include <string>

void Enforce(bool Condition, const char* Message);
std::vector<std::string> SplitByDelimiter(const std::string& Input, const std::string& Delimiter);
std::string Trim(const std::string& String);
std::string LowerCase(std::string Str);
int SpaceCount(const std::string& Str);
int WhiteSpaceCount(const std::string& Str);
bool IsValidToken(const std::string& Token);
bool IsValidURI(const std::string& URI);
bool IsValidHTTPVersion(const std::string& Version);
bool IsHexDigit(const char C);
int ParseHex(const std::string& Hex);
std::string TrimTrailingWhiteSpace(const std::string& String);
std::string URLDecode(const std::string& Encoded);
