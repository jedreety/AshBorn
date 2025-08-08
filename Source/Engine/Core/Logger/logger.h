#pragma once


#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

class Logger {
	public:
	enum class LogLevel {
		Info,
		Warning,
		Error
	};
	Logger(const std::string& filename) : logFile(filename, std::ios::app) {
		if (!logFile.is_open()) {
			throw std::runtime_error("Failed to open log file: " + filename);
		}
	}
	~Logger() {
		if (logFile.is_open()) {
			logFile.close();
		}
	}
	void log(LogLevel level, const std::string& message) {
		std::lock_guard<std::mutex> lock(mutex);
		logFile << getCurrentTime() << " [" << toString(level) << "] " << message << std::endl;
	}
	std::string getCurrentTime() {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm buf;
		localtime_s(&buf, &in_time_t);
		std::ostringstream oss;
		oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}
	std::string toString(LogLevel level) {
		switch (level) {
			case LogLevel::Info: return "INFO";
			case LogLevel::Warning: return "WARNING";
			case LogLevel::Error: return "ERROR";
			default: return "UNKNOWN";
		}
	}
private:
	std::ofstream logFile;
	std::mutex mutex;
	// Disable copy constructor and assignment operator
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
};
