#include "logging/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace
{
std::mutex& logMutex()
{
    static std::mutex mutex;
    return mutex;
}

void writeLog(const char* level, const std::string& message, std::ostream& stream)
{
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::lock_guard<std::mutex> lock(logMutex());
    stream << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "]["
           << level << "] " << message << '\n';
}
}

void Logger::info(const std::string& message)
{
    writeLog("INFO", message, std::cout);
}

void Logger::warn(const std::string& message)
{
    writeLog("WARN", message, std::cerr);
}

void Logger::error(const std::string& message)
{
    writeLog("ERROR", message, std::cerr);
}
