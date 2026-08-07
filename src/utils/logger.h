#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <string>
#include <stdexcept>
#include <iomanip>

enum LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

struct Logger {
    static LogLevel min_level;
    static std::ofstream log_file;
    static bool file_open;
    static int indent_level;
    
    static void init(const std::string& filename = "") {
        if (!filename.empty()) {
            log_file.open(filename, std::ios::out | std::ios::app);
            file_open = log_file.is_open();
        }
        indent_level = 0;
    }
    
    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    static std::string level_str(LogLevel level) {
        switch(level) {
            case DEBUG: return "DEBUG";
            case INFO:  return "INFO ";
            case WARN:  return "WARN ";
            case ERROR: return "ERROR";
            case FATAL: return "FATAL";
        }
        return "????";
    }
    
    static void log(LogLevel level, const std::string& msg) {
        if (level < min_level) return;
        
        std::string indent(indent_level * 2, ' ');
        std::stringstream ss;
        ss << "[" << timestamp() << "] "
           << "[" << level_str(level) << "] "
           << indent << msg;
        
        std::cout << ss.str() << std::endl;
        if (file_open) log_file << ss.str() << std::endl;
    }
    
    static void debug(const std::string& m) { log(DEBUG, m); }
    static void info(const std::string& m)  { log(INFO, m); }
    static void warn(const std::string& m)  { log(WARN, m); }
    static void error(const std::string& m) { log(ERROR, m); }
    static void fatal(const std::string& m) { log(FATAL, m); throw std::runtime_error(m); }
    
    static void header(const std::string& title) {
        std::string line = std::string(60, '=');
        info("");
        info(line);
        info("  " + title);
        info(line);
    }
    
    static void section(const std::string& title) {
        std::string line = std::string(60, '-');
        info("");
        info(line);
        info("  " + title);
        info(line);
    }
    
    static void keyval(const std::string& key, const std::string& val) {
        info("  " + key + ": " + val);
    }
    
    static void success(const std::string& msg) {
        info("[OK] " + msg);
    }
    
    static void failure(const std::string& msg) {
        error("[FAIL] " + msg);
    }
    
    static void increase_indent() { indent_level++; }
    static void decrease_indent() { if (indent_level > 0) indent_level--; }
    
    static void close() { 
        if (file_open) log_file.close(); 
    }
};

LogLevel Logger::min_level = INFO;
std::ofstream Logger::log_file;
bool Logger::file_open = false;
int Logger::indent_level = 0;
