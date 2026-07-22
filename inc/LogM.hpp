// ============================================================
//  How to use this logging utility:
//     logM(LogLevel::INFO, "Successfully connected to the server.");
//     logM(LogLevel::WARN, "Memory usage is high, consider releasing resources.");
//     logM(LogLevel::ERROR, "Failed to connect to the database!");
// ============================================================
#ifndef LOGM_HPP
#define LOGM_HPP

#include <iostream>
#include <string>

enum class LogLevel
{
    INFO,
    WARN,
    ERROR
};

void logM(LogLevel level, const std::string &message)
{
    switch (level)
    {
    case LogLevel::INFO:
        std::cout << "[INFO] " << message << std::endl;
        break;
    case LogLevel::WARN:
        std::cout << "[WARNING] " << message << std::endl;
        break;
    case LogLevel::ERROR:
        std::cerr << "[ERROR] " << message << std::endl;
        break;
    }
}

#endif // LOGM_HPP