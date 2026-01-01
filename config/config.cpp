#include "config.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigManager& ConfigManager::GetInstance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::Initialize()
{
    if (m_initialized)
        return true;
    
    // Get %appdata% path
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
    {
        return false;
    }
    
    m_configDir = std::string(appDataPath) + "\\cubixdlc";
    m_configPath = m_configDir + "\\config.ini";
    
    // Create directory if it doesn't exist
    CreateDirectoryA(m_configDir.c_str(), NULL);
    
    m_initialized = true;
    return true;
}

bool ConfigManager::LoadConfig()
{
    if (!m_initialized && !Initialize())
        return false;
    
    std::ifstream file(m_configPath);
    if (!file.is_open())
    {
        // Config doesn't exist, use defaults
        return false;
    }
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line))
    {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        
        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;
        
        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        
        // Trim key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        if (currentSection == "Watermark")
        {
            if (key == "enabled")
                m_config.watermark.enabled = (value == "1" || value == "true");
            else if (key == "showFPS")
                m_config.watermark.showFPS = (value == "1" || value == "true");
            else if (key == "posX")
                m_config.watermark.posX = (float)atof(value.c_str());
            else if (key == "posY")
                m_config.watermark.posY = (float)atof(value.c_str());
        }
        else if (currentSection.find("Category_") == 0)
        {
            // Parse category name from section
            std::string catName = currentSection.substr(9); // Skip "Category_"
            
            // Find or create category
            CategoryState* cat = nullptr;
            for (auto& c : m_config.categories)
            {
                if (c.name == catName)
                {
                    cat = &c;
                    break;
                }
            }
            
            if (!cat)
            {
                CategoryState newCat;
                newCat.name = catName;
                m_config.categories.push_back(newCat);
                cat = &m_config.categories.back();
            }
            
            if (key == "posX")
                cat->posX = (float)atof(value.c_str());
            else if (key == "posY")
                cat->posY = (float)atof(value.c_str());
            else if (key == "scrollOffset")
                cat->scrollOffset = (float)atof(value.c_str());
            else if (key.find("Module_") == 0)
            {
                // Module setting: Module_<name>_<setting>
                std::string moduleKey = key.substr(7); // Skip "Module_"
                size_t underscore = moduleKey.find('_');
                if (underscore != std::string::npos)
                {
                    std::string moduleName = moduleKey.substr(0, underscore);
                    std::string settingName = moduleKey.substr(underscore + 1);
                    
                    // Find or create module
                    ModuleState* mod = nullptr;
                    for (auto& m : cat->modules)
                    {
                        if (m.name == moduleName)
                        {
                            mod = &m;
                            break;
                        }
                    }
                    
                    if (!mod)
                    {
                        ModuleState newMod;
                        newMod.name = moduleName;
                        cat->modules.push_back(newMod);
                        mod = &cat->modules.back();
                    }
                    
                    if (settingName == "enabled")
                        mod->enabled = (value == "1" || value == "true");
                    else if (settingName == "expanded")
                        mod->expanded = (value == "1" || value == "true");
                    else if (settingName.find("setting_") == 0)
                    {
                        // Setting index
                        int settingIndex = atoi(settingName.substr(8).c_str());
                        if (settingIndex >= 0)
                        {
                            if (mod->settings.size() <= (size_t)settingIndex)
                                mod->settings.resize(settingIndex + 1);
                            mod->settings[settingIndex] = (value == "1" || value == "true");
                        }
                    }
                }
            }
        }
    }
    
    file.close();
    return true;
}

bool ConfigManager::SaveConfig()
{
    if (!m_initialized && !Initialize())
        return false;
    
    std::ofstream file(m_configPath);
    if (!file.is_open())
        return false;
    
    file << "; CubixDLC Configuration File\n";
    file << "; Auto-generated, do not edit manually\n\n";
    
    // Save watermark
    file << "[Watermark]\n";
    file << "enabled=" << (m_config.watermark.enabled ? "1" : "0") << "\n";
    file << "showFPS=" << (m_config.watermark.showFPS ? "1" : "0") << "\n";
    file << "posX=" << m_config.watermark.posX << "\n";
    file << "posY=" << m_config.watermark.posY << "\n\n";
    
    // Save categories
    for (const auto& cat : m_config.categories)
    {
        file << "[Category_" << cat.name << "]\n";
        file << "posX=" << cat.posX << "\n";
        file << "posY=" << cat.posY << "\n";
        file << "scrollOffset=" << cat.scrollOffset << "\n";
        
        for (const auto& mod : cat.modules)
        {
            file << "Module_" << mod.name << "_enabled=" << (mod.enabled ? "1" : "0") << "\n";
            file << "Module_" << mod.name << "_expanded=" << (mod.expanded ? "1" : "0") << "\n";
            
            for (size_t i = 0; i < mod.settings.size(); i++)
            {
                file << "Module_" << mod.name << "_setting_" << i << "=" << (mod.settings[i] ? "1" : "0") << "\n";
            }
        }
        
        file << "\n";
    }
    
    file.close();
    return true;
}

std::string ConfigManager::GetConfigPath() const
{
    return m_configPath;
}

std::string ConfigManager::GetConfigDir() const
{
    return m_configDir;
}

