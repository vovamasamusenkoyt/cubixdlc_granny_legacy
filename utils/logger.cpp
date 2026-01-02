#include "logger.h"
#include <windows.h>
#include <shlobj.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace CubixDLC
{
    namespace Logger
    {
        Logger& Logger::GetInstance()
        {
            static Logger instance;
            return instance;
        }

        bool Logger::Initialize()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            // Get %appdata% path
            char appDataPath[MAX_PATH];
            if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
            {
                return false;
            }
            
            // Create logs directory
            std::string logsDir = std::string(appDataPath) + "\\cubixdlc\\logs";
            CreateDirectoryA(logsDir.c_str(), NULL);
            
            // Create log file with timestamp
            std::time_t now = std::time(nullptr);
            std::tm timeInfo;
            localtime_s(&timeInfo, &now);
            
            std::ostringstream oss;
            oss << logsDir << "\\log_"
                << std::setfill('0') << std::setw(4) << (timeInfo.tm_year + 1900)
                << std::setw(2) << (timeInfo.tm_mon + 1)
                << std::setw(2) << timeInfo.tm_mday
                << "_"
                << std::setw(2) << timeInfo.tm_hour
                << std::setw(2) << timeInfo.tm_min
                << std::setw(2) << timeInfo.tm_sec
                << ".txt";
            
            m_LogFilePath = oss.str();
            m_LogFile.open(m_LogFilePath, std::ios::app);
            
            if (!m_LogFile.is_open())
            {
                return false;
            }
            
            LogInfo("Logger initialized. Log file: %s", m_LogFilePath.c_str());
            return true;
        }

        void Logger::Shutdown()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_LogFile.is_open())
            {
                LogInfo("Logger shutting down.");
                m_LogFile.close();
            }
        }

        Logger::~Logger()
        {
            Shutdown();
        }

        void Logger::Log(LogLevel level, const char* format, ...)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            va_list args;
            va_start(args, format);
            
            char buffer[4096];
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
            
            va_end(args);
            
            std::string message = GetCurrentTimeString() + " [" + GetLogLevelString(level) + "] " + buffer;
            
            // Write to file
            WriteToFile(level, message);
            
            // Add to in-memory log for console display
            m_LogMessages.push_back(message);
            if (m_LogMessages.size() > MAX_LOG_MESSAGES)
            {
                m_LogMessages.erase(m_LogMessages.begin());
            }
        }

        void Logger::LogDebug(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            char buffer[4096];
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
            va_end(args);
            Log(LogLevel::Debug, buffer);
        }

        void Logger::LogInfo(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            char buffer[4096];
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
            va_end(args);
            Log(LogLevel::Info, buffer);
        }

        void Logger::LogWarning(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            char buffer[4096];
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
            va_end(args);
            Log(LogLevel::Warning, buffer);
        }

        void Logger::LogError(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            char buffer[4096];
            vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
            va_end(args);
            Log(LogLevel::Error, buffer);
        }

        void Logger::WriteToFile(LogLevel /*level*/, const std::string& message)
        {
            if (m_LogFile.is_open())
            {
                m_LogFile << message << std::endl;
                m_LogFile.flush();
            }
        }

        std::string Logger::GetLogLevelString(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            default: return "UNKNOWN";
            }
        }

        std::string Logger::GetCurrentTimeString()
        {
            std::time_t now = std::time(nullptr);
            std::tm timeInfo;
            localtime_s(&timeInfo, &now);
            
            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(2) << timeInfo.tm_hour << ":"
                << std::setw(2) << timeInfo.tm_min << ":"
                << std::setw(2) << timeInfo.tm_sec;
            
            return oss.str();
        }
    }
}

