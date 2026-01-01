#pragma once

#include <string>
#include <vector>

// Module state
struct ModuleState {
    std::string name;
    bool enabled;
    bool expanded;
    std::vector<bool> settings; // For checkbox settings
};

// Category state
struct CategoryState {
    std::string name;
    float posX;
    float posY;
    float scrollOffset;
    std::vector<ModuleState> modules;
};

// Watermark state
struct WatermarkState {
    bool enabled;
    bool showFPS;
    float posX;
    float posY;
};

// Config structure
struct Config {
    std::vector<CategoryState> categories;
    WatermarkState watermark;
};

// Config management
class ConfigManager {
public:
    static ConfigManager& GetInstance();
    
    // Initialize config directory
    bool Initialize();
    
    // Load config from file
    bool LoadConfig();
    
    // Save config to file
    bool SaveConfig();
    
    // Get config path
    std::string GetConfigPath() const;
    std::string GetConfigDir() const;
    
    // Get current config
    Config& GetConfig() { return m_config; }
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    Config m_config;
    std::string m_configDir;
    std::string m_configPath;
    bool m_initialized;
};

