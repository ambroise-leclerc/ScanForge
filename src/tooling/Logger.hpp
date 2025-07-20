#pragma once

#include <iostream>
#include <string>
#include <format>
#include <chrono>
#include <source_location>

namespace scanforge::tooling {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void setLevel(LogLevel level) {
        current_level_ = level;
    }
    
    LogLevel getLevel() const {
        return current_level_;
    }
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < current_level_) {
            return;
        }
        
        auto now = std::chrono::system_clock::now();
        std::string level_str = getLevelString(level);
        std::string message;
        
        if constexpr (sizeof...(args) > 0) {
            message = std::vformat(format, std::make_format_args(args...));
        } else {
            message = format;
        }
        
        std::cout << std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}\n", 
                                std::chrono::floor<std::chrono::seconds>(now), 
                                level_str, message);
    }
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        log(LogLevel::INFO, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        log(LogLevel::WARNING, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        log(LogLevel::ERROR, format, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    LogLevel current_level_ = LogLevel::INFO;
    
    std::string getLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// Convenience macros
#define LOG_DEBUG(...) scanforge::tooling::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) scanforge::tooling::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...) scanforge::tooling::Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) scanforge::tooling::Logger::getInstance().error(__VA_ARGS__)

} // namespace scanforge::tooling
