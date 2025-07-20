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
    
    static void setLevel(LogLevel level) {
        getInstance().current_level_ = level;
    }
    
    static LogLevel getLevel() {
        return getInstance().current_level_;
    }
    
    template<typename... Args>
    static void log(LogLevel level, const std::string& format, Args&&... args) {
        auto& instance = getInstance();
        if (level < instance.current_level_) {
            return;
        }
        
        auto now = std::chrono::system_clock::now();
        std::string level_str = instance.getLevelString(level);
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
    static void debug(const std::string& format, Args&&... args) {
        log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        log(LogLevel::INFO, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warning(const std::string& format, Args&&... args) {
        log(LogLevel::WARNING, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
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

using Log = Logger;

} // namespace scanforge::tooling
