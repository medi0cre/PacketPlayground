#pragma once
#include <string>

#define Reset "\033[0m"
#define Gray "\033[90m"
#define Blue "\033[94m"
#define Green "\033[92m"
#define Orange "\033[38;5;208m"
#define Red "\033[91m"
#define RedBG "\033[41m"

enum LogLevel
{
    LogTrace,
    LogDebug,
    LogInfo,
    LogWarning,
    LogError,
    LogCritical
};

class Logger
{
public:
    static Logger& Get();
    void SetLevel(LogLevel L);

    void Trace(const std::string& Message);
    void Debug(const std::string& Message);
    void Info(const std::string& Message);
    void Warn(const std::string& Message);
    void Error(const std::string& Message);
    void Critical(const std::string& Message);

    Logger(const Logger&) = delete;
    Logger& operator = (const Logger&) = delete;

private:
    LogLevel Level = LogLevel::LogInfo;

    Logger() = default;
    ~Logger() = default;
};
