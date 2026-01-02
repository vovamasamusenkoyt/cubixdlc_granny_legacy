#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <memory>

namespace CubixDLC
{
    namespace Logger
    {
        enum class LogLevel
        {
            Debug,
            Info,
            Warning,
            Error
        };

        class Logger
        {
        public:
            static Logger& GetInstance();
            
            bool Initialize();
            void Shutdown();
            
            void Log(LogLevel level, const char* format, ...);
            void LogDebug(const char* format, ...);
            void LogInfo(const char* format, ...);
            void LogWarning(const char* format, ...);
            void LogError(const char* format, ...);
            
            // Get log messages for console display
            const std::vector<std::string>& GetLogMessages() const { return m_LogMessages; }
            void ClearLogMessages() { m_LogMessages.clear(); }
            
        private:
            Logger() = default;
            ~Logger();
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;
            
            void WriteToFile(LogLevel level, const std::string& message);
            std::string GetLogLevelString(LogLevel level);
            std::string GetCurrentTimeString();
            
            std::ofstream m_LogFile;
            std::string m_LogFilePath;
            std::mutex m_Mutex;
            std::vector<std::string> m_LogMessages;
            static const size_t MAX_LOG_MESSAGES = 1000;
        };
    }
}

// Convenience macros
#define LOG_DEBUG(...) CubixDLC::Logger::Logger::GetInstance().LogDebug(__VA_ARGS__)
#define LOG_INFO(...) CubixDLC::Logger::Logger::GetInstance().LogInfo(__VA_ARGS__)
#define LOG_WARNING(...) CubixDLC::Logger::Logger::GetInstance().LogWarning(__VA_ARGS__)
#define LOG_ERROR(...) CubixDLC::Logger::Logger::GetInstance().LogError(__VA_ARGS__)

