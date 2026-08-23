#include <Logger.h>
#include <iostream>

Logger& Logger::Get()
{
    static Logger L;
    return L;
}

void Logger::SetLevel(LogLevel L)
{
    if (L < LogLevel::LogTrace || L > LogLevel::LogCritical) { return; }
    Level = L;
}

void Logger::Trace(const std::string& Message)
{
    if (Level > LogLevel::LogTrace) { return; }
    std::cerr << "[" Gray "TRACE" Reset "] " + Message + "\n";
}

void Logger::Debug(const std::string& Message)
{
    if (Level > LogLevel::LogDebug) { return; }
    std::cerr << "[" Blue "DEBUG" Reset "] " + Message + "\n";
}

void Logger::Info(const std::string& Message)
{
    if (Level > LogLevel::LogInfo) { return; }
    std::cerr << "[" Green "INFO" Reset "] " + Message + "\n";
}

void Logger::Warn(const std::string& Message)
{
    if (Level > LogLevel::LogWarning) { return; }
    std::cerr << "[" Orange "WARNING" Reset "] " + Message + "\n";
}

void Logger::Error(const std::string& Message)
{
    if (Level > LogLevel::LogError) { return; }
    std::cerr << "[" Red "ERROR" Reset "] " + Message + "\n";
}

void Logger::Critical(const std::string& Message)
{
    if (Level > LogLevel::LogCritical) { return; }
    std::cerr << "[" RedBG "CRITICAL" Reset "] " + Message + "\n";
}
