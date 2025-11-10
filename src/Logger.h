#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>

#define LOG(message) Logger::getInstance().log(message)

// Simple file logger
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> guard(logMutex);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            char timestamp[26];
            ctime_s(timestamp, sizeof(timestamp), &time);
            timestamp[24] = '\0'; // Remove newline
            logFile << "[" << timestamp << "] " << message << std::endl;
        }
    }

private:
    Logger() {
        logFile.open("ultravox.log", std::ios::out | std::ios::trunc);
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::mutex logMutex;
};