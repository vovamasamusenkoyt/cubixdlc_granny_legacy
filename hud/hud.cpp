#include "hud.h"
#include "../deps/imgui/imgui.h"
#include "../hook_render/hook_render.h"
#include "../modules/watermark/watermark.h"
#include "../config/config.h"
#include "../modules/esp/esp.h"
#include "../modules/noclip/noclip.h"
#include "../modules/invisible/invisible.h"
#include "../modules/escapes/escapes.h"
#include "../modules/itemspawner/itemspawner.h"
#include "../utils/logger.h"
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Module structure
struct Module {
    std::string name;
    bool enabled;
    bool expanded;
    std::vector<std::string> settings;
    int keybind;  // Virtual key code, 0 = no keybind
    
    Module(const char* n) : name(n), enabled(false), expanded(false), keybind(0) {}
};

// Keybind editing state
static bool g_WaitingForKeybind = false;
static std::string g_KeybindTargetModule = "";
static size_t g_KeybindTargetCategory = 0;

// Item Browser state
static bool g_ShowItemBrowser = false;
static CubixDLC::ItemSpawner::ItemCategory g_CurrentItemCategory = CubixDLC::ItemSpawner::ItemCategory::All;
static float g_ItemBrowserScroll = 0.0f;

// Get key name from virtual key code
const char* GetKeyName(int vk)
{
    static char keyName[32];
    if (vk == 0) return "None";
    
    switch (vk)
    {
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        case VK_NUMPAD0: return "Num0";
        case VK_NUMPAD1: return "Num1";
        case VK_NUMPAD2: return "Num2";
        case VK_NUMPAD3: return "Num3";
        case VK_NUMPAD4: return "Num4";
        case VK_NUMPAD5: return "Num5";
        case VK_NUMPAD6: return "Num6";
        case VK_NUMPAD7: return "Num7";
        case VK_NUMPAD8: return "Num8";
        case VK_NUMPAD9: return "Num9";
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PgUp";
        case VK_NEXT: return "PgDn";
        case VK_ESCAPE: return "Esc";
        default:
            if (vk >= 'A' && vk <= 'Z')
            {
                keyName[0] = (char)vk;
                keyName[1] = 0;
                return keyName;
            }
            if (vk >= '0' && vk <= '9')
            {
                keyName[0] = (char)vk;
                keyName[1] = 0;
                return keyName;
            }
            snprintf(keyName, sizeof(keyName), "0x%02X", vk);
            return keyName;
    }
}

// Category structure
struct Category {
    std::string name;
    std::vector<Module> modules;
    float scrollOffset;
    float scrollTarget;
    float posX;
    float posY;
    bool isDragging;
    float dragOffsetX;
    float dragOffsetY;
    
    Category(const char* n) : name(n), scrollOffset(0.0f), scrollTarget(0.0f), posX(0.0f), posY(0.0f), isDragging(false), dragOffsetX(0.0f), dragOffsetY(0.0f) {}
};

// Global categories
static std::vector<Category> g_Categories;
static bool g_CategoriesInitialized = false;
static bool g_ConfigLoaded = false;

// Constants (increased by 50% from base, which was already 25% larger)
const float PANEL_WIDTH = 234.375f;  // 125 * 1.25 * 1.5
const float PANEL_HEIGHT = 525.0f;  // 280 * 1.25 * 1.5
const float PANEL_MARGIN = 12.0f;  // 8 * 1.5
const float TITLE_HEIGHT = 30.0f;  // 20 * 1.5
const float TITLE_MARGIN_TOP = 7.5f;  // 5 * 1.5
const float FUNCTION_HEIGHT = 30.0f;  // 20 * 1.5
const float SCROLL_AREA_Y_OFFSET = TITLE_MARGIN_TOP + TITLE_HEIGHT;
const float SCROLL_AREA_HEIGHT = PANEL_HEIGHT - SCROLL_AREA_Y_OFFSET - 7.5f;  // 5 * 1.5
const float SCROLL_SPEED = 18.0f;  // 12 * 1.5
const float SCROLL_LERP_FACTOR = 20.0f;

// Animation progress for modules
static std::map<std::string, float> g_ExpandProgress;

// Purple color for enabled modules
static ImU32 GetPurpleColor() {
    return IM_COL32(147, 51, 234, 255); // Purple color
}

// Play sound from embedded resources
void PlaySoundResource(int resourceId)
{
    HMODULE hModule = GetModuleHandleA("cubixdlc.dll");
    if (!hModule)
    {
        // Fallback: get module from current address
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&PlaySoundResource, &hModule);
    }
    
    if (hModule)
    {
        PlaySoundA(MAKEINTRESOURCEA(resourceId), hModule, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
    }
}

// Load config into categories
void LoadConfigToCategories()
{
    if (g_ConfigLoaded) return;
    
    ConfigManager& configMgr = ConfigManager::GetInstance();
    if (!configMgr.Initialize())
        return;
    
    bool configLoaded = configMgr.LoadConfig();
    Config& config = configMgr.GetConfig();
    
    // Load watermark settings
    WatermarkSettings& wmSettings = GetWatermarkSettings();
    if (configLoaded)
    {
        wmSettings.enabled = config.watermark.enabled;
        wmSettings.showFPS = config.watermark.showFPS;
        // Only use saved position if it's not zero (default)
        if (config.watermark.posX > 0.0f || config.watermark.posY > 0.0f)
        {
            wmSettings.posX = config.watermark.posX;
            wmSettings.posY = config.watermark.posY;
        }
    }
    
    g_ConfigLoaded = true;
}

// Save categories to config
void SaveCategoriesToConfig()
{
    ConfigManager& configMgr = ConfigManager::GetInstance();
    if (!configMgr.Initialize())
        return;
    
    Config& config = configMgr.GetConfig();
    
    // Save watermark settings
    WatermarkSettings& wmSettings = GetWatermarkSettings();
    config.watermark.enabled = wmSettings.enabled;
    config.watermark.showFPS = wmSettings.showFPS;
    config.watermark.posX = wmSettings.posX;
    config.watermark.posY = wmSettings.posY;
    
    // Save categories
    config.categories.clear();
    for (const auto& cat : g_Categories)
    {
        CategoryState catState;
        catState.name = cat.name;
        catState.posX = cat.posX;
        catState.posY = cat.posY;
        catState.scrollOffset = cat.scrollOffset;
        
        for (const auto& mod : cat.modules)
        {
            ModuleState modState;
            modState.name = mod.name;
            modState.enabled = mod.enabled;
            modState.expanded = mod.expanded;
            modState.keybind = mod.keybind;
            
            // Save settings (for watermark Show FPS)
            if (mod.name == "Watermark" && mod.settings.size() > 0)
            {
                modState.settings.push_back(wmSettings.showFPS);
            }
            
            catState.modules.push_back(modState);
        }
        
        config.categories.push_back(catState);
    }
    
    configMgr.SaveConfig();
}

// Initialize categories
void InitializeCategories()
{
    if (g_CategoriesInitialized) return;
    
    // Load config first
    LoadConfigToCategories();
    
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float totalWidth = 5 * (PANEL_WIDTH + PANEL_MARGIN) - PANEL_MARGIN;
    float startX = (screenSize.x - totalWidth) * 0.5f;
    float startY = (screenSize.y - PANEL_HEIGHT) * 0.5f;
    
    ConfigManager& configMgr = ConfigManager::GetInstance();
    Config& config = configMgr.GetConfig();
    
    // Try to load from config
    bool loadedFromConfig = false;
    if (!config.categories.empty())
    {
        // Load categories from config
        for (const auto& catState : config.categories)
        {
            Category cat(catState.name.c_str());
            cat.posX = catState.posX;
            cat.posY = catState.posY;
            cat.scrollOffset = catState.scrollOffset;
            
            for (const auto& modState : catState.modules)
            {
                Module mod(modState.name.c_str());
                mod.enabled = modState.enabled;
                mod.expanded = modState.expanded;
                mod.keybind = modState.keybind;
                
                // Load settings
                if (modState.name == "Watermark")
                {
                    mod.settings.push_back("Show FPS");
                    if (modState.settings.size() > 0)
                    {
                        GetWatermarkSettings().showFPS = modState.settings[0];
                    }
                }
                else if (modState.name == "ESP")
                {
                    // Add ESP settings (only Granny and Grandpa)
                    mod.settings.push_back("Show Box");
                    mod.settings.push_back("Show Name");
                    mod.settings.push_back("Show Distance");
                    mod.settings.push_back("Show Granny");
                    mod.settings.push_back("Show Grandpa");
                    mod.settings.push_back("Granny Color");
                    mod.settings.push_back("Grandpa Color");
                }
                else if (modState.name == "NoClip")
                {
                    mod.settings.push_back("Speed");
                    mod.settings.push_back("Bind");
                }
                
                // Add Bind setting to toggleable modules (not escape buttons)
                bool isEscapeButton = (modState.name == "Door" || modState.name == "Car" || 
                                      modState.name == "Cellar" || modState.name == "Robot");
                if (!isEscapeButton && modState.name != "NoClip")  // NoClip already has Bind above
                {
                    // Check if Bind is not already added
                    bool hasBind = false;
                    for (const auto& s : mod.settings)
                    {
                        if (s == "Bind") { hasBind = true; break; }
                    }
                    if (!hasBind)
                    {
                        mod.settings.push_back("Bind");
                    }
                }
                
                cat.modules.push_back(mod);
            }
            
            g_Categories.push_back(cat);
        }
        loadedFromConfig = true;
        
        // Ensure NoClip exists in Movement category (for configs created before NoClip was added)
        for (auto& cat : g_Categories)
        {
            if (cat.name == "Movement")
            {
                bool hasNoClip = false;
                for (const auto& mod : cat.modules)
                {
                    if (mod.name == "NoClip")
                    {
                        hasNoClip = true;
                        break;
                    }
                }
                if (!hasNoClip)
                {
                    Module noclipMod("NoClip");
                    noclipMod.settings.push_back("Speed");
                    noclipMod.settings.push_back("Bind");
                    cat.modules.push_back(noclipMod);
                }
                break;
            }
        }
        
        // Ensure Invisible exists in Player category and remove old modules
        for (auto& cat : g_Categories)
        {
            if (cat.name == "Player")
            {
                // Remove old Infinite Ammo and Give Items modules
                cat.modules.erase(
                    std::remove_if(cat.modules.begin(), cat.modules.end(),
                        [](const Module& m) { 
                            return m.name == "Infinite Ammo" || m.name == "Give Items"; 
                        }),
                    cat.modules.end());
                
                // Add Invisible if not present
                bool hasInvisible = false;
                for (const auto& mod : cat.modules)
                {
                    if (mod.name == "Invisible")
                    {
                        hasInvisible = true;
                        break;
                    }
                }
                if (!hasInvisible)
                {
                    cat.modules.push_back(Module("Invisible"));
                }
                break;
            }
        }
        
        // Remove Items category if it exists (old config)
        g_Categories.erase(
            std::remove_if(g_Categories.begin(), g_Categories.end(),
                [](const Category& c) { return c.name == "Items"; }),
            g_Categories.end());
        
        // Remove Main category if it exists (old config with Coins)
        g_Categories.erase(
            std::remove_if(g_Categories.begin(), g_Categories.end(),
                [](const Category& c) { return c.name == "Main"; }),
            g_Categories.end());
        
        // Ensure Escapes category exists
        bool hasEscapesCategory = false;
        for (const auto& cat : g_Categories)
        {
            if (cat.name == "Escapes")
            {
                hasEscapesCategory = true;
                break;
            }
        }
        if (!hasEscapesCategory)
        {
            Category escapesCat("Escapes");
            escapesCat.posX = 50.0f;
            escapesCat.posY = 330.0f;
            escapesCat.modules.push_back(Module("Door"));
            escapesCat.modules.push_back(Module("Car"));
            escapesCat.modules.push_back(Module("Cellar"));
            escapesCat.modules.push_back(Module("Robot"));
            g_Categories.push_back(escapesCat);
        }
    }
    
    // If not loaded from config, create default categories
    if (!loadedFromConfig)
    {
        // Combat category
        Category combat("Combat");
        combat.posX = startX;
        combat.posY = startY;
        combat.modules.push_back(Module("Aim"));
        combat.modules.push_back(Module("Silent"));
        combat.modules.push_back(Module("No Recoil"));
        combat.modules.push_back(Module("No Spread"));
        g_Categories.push_back(combat);
        
        // Movement category
        Category movement("Movement");
        movement.posX = startX + (PANEL_WIDTH + PANEL_MARGIN);
        movement.posY = startY;
        movement.modules.push_back(Module("Speedhack"));
        movement.modules.push_back(Module("Fly"));
        movement.modules.push_back(Module("No Fall"));
        {
            Module noclipMod("NoClip");
            noclipMod.settings.push_back("Speed");
            noclipMod.settings.push_back("Bind");
            movement.modules.push_back(noclipMod);
        }
        g_Categories.push_back(movement);
        
        // Render category
        Category render("Render");
        render.posX = startX + 2 * (PANEL_WIDTH + PANEL_MARGIN);
        render.posY = startY;
        Module espMod("ESP");
        espMod.settings.push_back("Show Box");
        espMod.settings.push_back("Show Name");
        espMod.settings.push_back("Show Distance");
        espMod.settings.push_back("Show Granny");
        espMod.settings.push_back("Show Grandpa");
        espMod.settings.push_back("Granny Color");
        espMod.settings.push_back("Grandpa Color");
        render.modules.push_back(espMod);
        render.modules.push_back(Module("Chams"));
        render.modules.push_back(Module("Wallhack"));
        render.modules.push_back(Module("Tracers"));
        g_Categories.push_back(render);
        
        // Player category
        Category player("Player");
        player.posX = startX + 3 * (PANEL_WIDTH + PANEL_MARGIN);
        player.posY = startY;
        player.modules.push_back(Module("Godmode"));
        player.modules.push_back(Module("Invisible"));
        player.modules.push_back(Module("No Hunger"));
        g_Categories.push_back(player);
        
        // Misc category
        Category misc("Misc");
        misc.posX = startX + 4 * (PANEL_WIDTH + PANEL_MARGIN);
        misc.posY = startY;
        misc.modules.push_back(Module("Auto Clicker"));
        misc.modules.push_back(Module("Reach"));
        g_Categories.push_back(misc);
        
        // Visual category (for watermark and other visual modules)
        Category visual("Visual");
        visual.posX = startX + 5 * (PANEL_WIDTH + PANEL_MARGIN);
        visual.posY = startY;
        Module watermarkMod("Watermark");
        watermarkMod.settings.push_back("Show FPS");
        watermarkMod.enabled = true; // Watermark enabled by default
        visual.modules.push_back(watermarkMod);
        g_Categories.push_back(visual);
        
        // Escapes category (instant win options)
        Category escapesCat("Escapes");
        escapesCat.posX = startX;
        escapesCat.posY = startY + 280.0f;
        escapesCat.modules.push_back(Module("Door"));
        escapesCat.modules.push_back(Module("Car"));
        escapesCat.modules.push_back(Module("Cellar"));
        escapesCat.modules.push_back(Module("Robot"));
        g_Categories.push_back(escapesCat);
        
        // Update total width for 6 panels
        totalWidth = 6 * (PANEL_WIDTH + PANEL_MARGIN) - PANEL_MARGIN;
        startX = (screenSize.x - totalWidth) * 0.5f;
        
        // Recalculate positions for all categories
        for (size_t i = 0; i < g_Categories.size(); i++)
        {
            g_Categories[i].posX = startX + i * (PANEL_WIDTH + PANEL_MARGIN);
        }
    }
    
    g_CategoriesInitialized = true;
}

// Update expand animation
float UpdateExpandAnimation(const std::string& moduleName, bool expanded)
{
    float target = expanded ? 1.0f : 0.0f;
    float current = g_ExpandProgress[moduleName];
    current = current + (target - current) * 15.0f * ImGui::GetIO().DeltaTime;
    if (fabsf(target - current) < 0.001f) current = target;
    g_ExpandProgress[moduleName] = current;
    return current;
}

// Calculate max scroll for category
float CalculateMaxScroll(const Category& cat)
{
    float totalHeight = 0.0f;
    for (const auto& mod : cat.modules)
    {
        float prog = g_ExpandProgress[mod.name];
        float settingsHeight = mod.settings.size() * 27.0f * prog;  // 18 * 1.5
        totalHeight += FUNCTION_HEIGHT + settingsHeight;
    }
    float maxScroll = totalHeight - SCROLL_AREA_HEIGHT;
    return (std::max)(0.0f, maxScroll);
}

// Render scrollbar
void RenderScrollbar(ImDrawList* draw_list, float x, float y, const Category& cat)
{
    float maxScroll = CalculateMaxScroll(cat);
    if (maxScroll <= 0.0f) return;
    
    float scrollbarWidth = 4.5f;  // 3 * 1.5
    float scrollbarX = x + PANEL_WIDTH - scrollbarWidth - 1.5f;  // 1 * 1.5
    float scrollbarHeight = SCROLL_AREA_HEIGHT - 45.0f;  // 30 * 1.5
    float scrollbarY = y + SCROLL_AREA_Y_OFFSET + 22.5f;  // 15 * 1.5
    
    // Scrollbar background
    ImU32 bgColor = IM_COL32(0, 0, 0, 50);
    draw_list->AddRectFilled(ImVec2(scrollbarX, scrollbarY), ImVec2(scrollbarX + scrollbarWidth, scrollbarY + scrollbarHeight), bgColor, 1.0f);
    
    // Scrollbar thumb
    float scrollProgress = cat.scrollOffset / maxScroll;
    float thumbHeight = (std::max)(6.0f, scrollbarHeight * (SCROLL_AREA_HEIGHT / (SCROLL_AREA_HEIGHT + maxScroll)));
    float thumbY = scrollbarY + scrollProgress * (scrollbarHeight - thumbHeight);
    
    ImU32 thumbColor = IM_COL32(255, 255, 255, 150);
    draw_list->AddRectFilled(ImVec2(scrollbarX, thumbY), ImVec2(scrollbarX + scrollbarWidth, thumbY + thumbHeight), thumbColor, 1.0f);
}

// Render panel
void RenderPanel(ImDrawList* draw_list, float x, float y, Category& cat, float, size_t catIdx)
{
    // Panel background (Exosware dark color: 17, 15, 28)
    ImU32 panelColor = IM_COL32(17, 15, 28, 200);
    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + PANEL_WIDTH, y + PANEL_HEIGHT), panelColor, 18.0f);  // 12 * 1.5
    
    // Panel border
    ImU32 borderColor = IM_COL32(40, 40, 50, 150);
    draw_list->AddRect(ImVec2(x, y), ImVec2(x + PANEL_WIDTH, y + PANEL_HEIGHT), borderColor, 12.0f, 0, 1.5f);
    
    // Title
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();
    ImVec2 titleSize = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, cat.name.c_str());
    ImVec2 titlePos = ImVec2(x + (PANEL_WIDTH - titleSize.x) * 0.5f, y + TITLE_MARGIN_TOP + 3.0f);  // 2 * 1.5
    draw_list->AddText(font, font_size, titlePos, IM_COL32(255, 255, 255, 255), cat.name.c_str());
    
    // Update scroll
    cat.scrollOffset = cat.scrollOffset + (cat.scrollTarget - cat.scrollOffset) * SCROLL_LERP_FACTOR * ImGui::GetIO().DeltaTime;
    float maxScroll = CalculateMaxScroll(cat);
    cat.scrollTarget = (std::max)(0.0f, (std::min)(cat.scrollTarget, maxScroll));
    cat.scrollOffset = (std::max)(0.0f, (std::min)(cat.scrollOffset, maxScroll));
    
    // Render scrollbar
    RenderScrollbar(draw_list, x, y, cat);
    
    // Render modules
    float currentY = y + SCROLL_AREA_Y_OFFSET - cat.scrollOffset;
    ImVec2 clipMin = ImVec2(x, y + SCROLL_AREA_Y_OFFSET);
    ImVec2 clipMax = ImVec2(x + PANEL_WIDTH, y + SCROLL_AREA_Y_OFFSET + SCROLL_AREA_HEIGHT);
    
    for (auto& mod : cat.modules)
    {
        float prog = UpdateExpandAnimation(mod.name, mod.expanded);
        float settingsHeight = mod.settings.size() * 27.0f * prog;  // 18 * 1.5
        float totalHeight = FUNCTION_HEIGHT + settingsHeight;
        
        if (currentY + totalHeight < y + SCROLL_AREA_Y_OFFSET || currentY > y + PANEL_HEIGHT)
        {
            currentY += totalHeight;
            continue;
        }
        
        // Check if this is an escape button (in Escapes category)
        bool isEscapeButton = (mod.name == "Door" || mod.name == "Car" || mod.name == "Cellar" || mod.name == "Robot");
        
        // Module background (if enabled) - purple fill, or green for escape buttons
        ImU32 purpleColor = GetPurpleColor();
        ImU32 greenButtonColor = IM_COL32(60, 160, 80, 200);
        ImU32 greenButtonHover = IM_COL32(80, 200, 100, 220);
        ImU32 moduleBg = mod.enabled ? purpleColor : IM_COL32(198, 198, 198, 30);
        
        if (isEscapeButton)
        {
            // Escape buttons always show as clickable buttons
            ImVec2 mousePos = ImGui::GetMousePos();
            bool isHovered = (mousePos.x >= x + 6 && mousePos.x <= x + PANEL_WIDTH - 6 &&
                             mousePos.y >= currentY - 1.5f && mousePos.y <= currentY + FUNCTION_HEIGHT - 1.5f);
            draw_list->AddRectFilled(ImVec2(x + 6, currentY - 1.5f), ImVec2(x + PANEL_WIDTH - 6, currentY + FUNCTION_HEIGHT - 1.5f), 
                                    isHovered ? greenButtonHover : greenButtonColor, 6.0f);
        }
        else if (mod.enabled)
        {
            draw_list->AddRectFilled(ImVec2(x + 6, currentY - 1.5f), ImVec2(x + PANEL_WIDTH - 6, currentY + totalHeight - 1.5f), moduleBg, 6.0f);  // 4 * 1.5, 1 * 1.5
        }
        
        // Module name - white text if enabled, gray if disabled
        ImVec2 textPos = ImVec2(x + 15, currentY + 4.5f);  // 10 * 1.5, 3 * 1.5
        ImU32 textColor = (mod.enabled || isEscapeButton) ? IM_COL32(255, 255, 255, 255) : IM_COL32(198, 198, 198, 255);
        draw_list->AddText(font, font_size, textPos, textColor, mod.name.c_str());
        
        // Arrow for expandable modules
        if (!mod.settings.empty())
        {
            float arrowX = x + PANEL_WIDTH - 15;
            float arrowY = currentY + FUNCTION_HEIGHT * 0.5f;
            float angle = prog * 90.0f;
            float rad = angle * 3.14159f / 180.0f;
            
            ImVec2 arrowPos = ImVec2(arrowX, arrowY);
            ImVec2 arrowEnd = ImVec2(arrowX + 6.0f * cosf(rad), arrowY + 6.0f * sinf(rad));
            draw_list->AddLine(arrowPos, arrowEnd, textColor, 2.0f);
        }
        
        // Settings (if expanded)
        if (settingsHeight > 0.0f)
        {
            float settingY = currentY + FUNCTION_HEIGHT;
            for (size_t i = 0; i < mod.settings.size(); i++)
            {
                float alpha = prog;
                float settingLineY = settingY + i * 27.0f;  // 18 * 1.5
                ImVec2 settingPos = ImVec2(x + 37.5f, settingLineY + 3.0f);  // 25 * 1.5, 2 * 1.5
                ImU32 settingColor = IM_COL32(200, 200, 200, (int)(255 * alpha));
                
                // Get setting value for watermark module
                bool settingEnabled = false;
                if (mod.name == "Watermark" && mod.settings[i] == "Show FPS")
                {
                    settingEnabled = GetWatermarkSettings().showFPS;
                }
                // Get setting value for ESP module
                else if (mod.name == "ESP")
                {
                    auto& espSettings = CubixDLC::ESPEnemy::GetModule()->GetSettings();
                    
                    if (mod.settings[i] == "Show Box")
                        settingEnabled = espSettings.showBox;
                    else if (mod.settings[i] == "Show Name")
                        settingEnabled = espSettings.showName;
                    else if (mod.settings[i] == "Show Distance")
                        settingEnabled = espSettings.showDistance;
                    else if (mod.settings[i] == "Show Granny")
                        settingEnabled = espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Granny].enabled;
                    else if (mod.settings[i] == "Show Grandpa")
                        settingEnabled = espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Grandpa].enabled;
                    // Color settings are not checkboxes, they show color picker
                    else if (mod.settings[i].find("Color") != std::string::npos)
                        settingEnabled = false; // Color picker, not checkbox
                }
                
                // Check if this is a color picker setting
                bool isColorPicker = false;
                CubixDLC::ESPEnemy::EnemyType colorEnemyType = CubixDLC::ESPEnemy::EnemyType::Granny;
                if (mod.name == "ESP" && mod.settings[i].find("Color") != std::string::npos)
                {
                    isColorPicker = true;
                    // Determine enemy type from setting name
                    if (mod.settings[i] == "Granny Color")
                        colorEnemyType = CubixDLC::ESPEnemy::EnemyType::Granny;
                    else if (mod.settings[i] == "Grandpa Color")
                        colorEnemyType = CubixDLC::ESPEnemy::EnemyType::Grandpa;
                }
                
                // Check if this is a NoClip speed slider
                bool isSpeedSlider = (mod.name == "NoClip" && mod.settings[i] == "Speed");
                
                if (isColorPicker)
                {
                    // Draw color picker button
                    auto& espSettings = CubixDLC::ESPEnemy::GetModule()->GetSettings();
                    auto& colorSettings = espSettings.enemyColors[colorEnemyType];
                    ImVec4 color = colorSettings.color;
                    
                    float colorButtonX = x + 18.0f;
                    float colorButtonY = settingLineY + 3.0f;
                    float colorButtonSize = 15.0f;
                    ImU32 colorU32 = ImGui::ColorConvertFloat4ToU32(color);
                    
                    // Draw color button
                    draw_list->AddRectFilled(ImVec2(colorButtonX, colorButtonY), 
                                            ImVec2(colorButtonX + colorButtonSize, colorButtonY + colorButtonSize), 
                                            colorU32, 2.0f);
                    draw_list->AddRect(ImVec2(colorButtonX, colorButtonY), 
                                      ImVec2(colorButtonX + colorButtonSize, colorButtonY + colorButtonSize), 
                                      settingColor, 1.5f, 0, 1.0f);
                    
                    // Use ImGui InvisibleButton for click detection
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImGui::SetCursorPos(ImVec2(colorButtonX - windowPos.x, colorButtonY - windowPos.y));
                    ImGui::PushID((mod.name + "_" + mod.settings[i]).c_str());
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    
                    if (ImGui::InvisibleButton("##colorbtn", ImVec2(colorButtonSize, colorButtonSize)))
                    {
                        ImGui::OpenPopup("color_picker_popup");
                    }
                    
                    if (ImGui::BeginPopup("color_picker_popup"))
                    {
                        float col[3] = { color.x, color.y, color.z };
                        if (ImGui::ColorPicker3("##picker", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
                        {
                            colorSettings.color = ImVec4(col[0], col[1], col[2], 1.0f);
                            SaveCategoriesToConfig();
                        }
                        ImGui::EndPopup();
                    }
                    
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
                else if (isSpeedSlider)
                {
                    // Draw speed slider for NoClip
                    float sliderX = x + 18.0f;
                    float sliderY = settingLineY + 5.0f;
                    float sliderWidth = PANEL_WIDTH - 60.0f;
                    float sliderHeight = 12.0f;
                    
                    float currentSpeed = CubixDLC::NoClip::GetModule()->GetSpeed();
                    float minSpeed = 0.05f;
                    float maxSpeed = 1.5f;
                    float normalizedValue = (currentSpeed - minSpeed) / (maxSpeed - minSpeed);
                    
                    // Draw slider background
                    draw_list->AddRectFilled(ImVec2(sliderX, sliderY), 
                                            ImVec2(sliderX + sliderWidth, sliderY + sliderHeight), 
                                            IM_COL32(40, 40, 40, 200), 3.0f);
                    
                    // Draw slider fill
                    float fillWidth = sliderWidth * normalizedValue;
                    draw_list->AddRectFilled(ImVec2(sliderX, sliderY), 
                                            ImVec2(sliderX + fillWidth, sliderY + sliderHeight), 
                                            IM_COL32(80, 150, 255, (int)(200 * alpha)), 3.0f);
                    
                    // Draw slider border
                    draw_list->AddRect(ImVec2(sliderX, sliderY), 
                                      ImVec2(sliderX + sliderWidth, sliderY + sliderHeight), 
                                      settingColor, 3.0f, 0, 1.0f);
                    
                    // Draw speed value text
                    char speedText[32];
                    snprintf(speedText, sizeof(speedText), "%.2f", currentSpeed);
                    ImVec2 speedTextSize = font->CalcTextSizeA(font_size * 0.7f, FLT_MAX, 0.0f, speedText);
                    draw_list->AddText(font, font_size * 0.7f, 
                                      ImVec2(sliderX + sliderWidth + 5.0f, sliderY), 
                                      settingColor, speedText);
                    
                    // Handle slider interaction using ImGui
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImGui::SetCursorPos(ImVec2(sliderX - windowPos.x, sliderY - windowPos.y));
                    ImGui::PushID((mod.name + "_speed_slider").c_str());
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    
                    ImGui::InvisibleButton("##speedslider", ImVec2(sliderWidth, sliderHeight));
                    if (ImGui::IsItemActive())
                    {
                        ImVec2 mousePos = ImGui::GetMousePos();
                        float relativeX = mousePos.x - sliderX;
                        float newNormalized = relativeX / sliderWidth;
                        newNormalized = (newNormalized < 0.0f) ? 0.0f : ((newNormalized > 1.0f) ? 1.0f : newNormalized);
                        float newSpeed = minSpeed + newNormalized * (maxSpeed - minSpeed);
                        CubixDLC::NoClip::GetModule()->SetSpeed(newSpeed);
                    }
                    
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
                else if (mod.settings[i] == "Bind")
                {
                    // Draw Bind button
                    float bindX = x + 18.0f;
                    float bindY = settingLineY + 2.0f;
                    float bindWidth = PANEL_WIDTH - 60.0f;
                    float bindHeight = 18.0f;
                    
                    // Get current keybind name
                    const char* keyName = GetKeyName(mod.keybind);
                    char bindText[64];
                    snprintf(bindText, sizeof(bindText), "Bind: [%s]", keyName);
                    
                    // Button colors
                    bool isWaiting = (g_WaitingForKeybind && g_KeybindTargetModule == mod.name);
                    ImU32 bindBg = isWaiting ? IM_COL32(100, 80, 150, (int)(200 * alpha)) 
                                            : IM_COL32(50, 50, 70, (int)(180 * alpha));
                    ImU32 bindHover = IM_COL32(70, 70, 100, (int)(220 * alpha));
                    
                    // Check hover
                    ImVec2 mousePos = ImGui::GetMousePos();
                    bool isHovered = (mousePos.x >= bindX && mousePos.x <= bindX + bindWidth &&
                                     mousePos.y >= bindY && mousePos.y <= bindY + bindHeight);
                    
                    // Draw button
                    draw_list->AddRectFilled(ImVec2(bindX, bindY), 
                                            ImVec2(bindX + bindWidth, bindY + bindHeight), 
                                            isHovered ? bindHover : bindBg, 4.0f);
                    draw_list->AddRect(ImVec2(bindX, bindY), 
                                      ImVec2(bindX + bindWidth, bindY + bindHeight), 
                                      settingColor, 4.0f, 0, 1.0f);
                    
                    // Draw text centered
                    const char* displayText = isWaiting ? "Press a key..." : bindText;
                    ImVec2 textSize = font->CalcTextSizeA(font_size * 0.75f, FLT_MAX, 0.0f, displayText);
                    float textX = bindX + (bindWidth - textSize.x) * 0.5f;
                    float textY = bindY + (bindHeight - textSize.y) * 0.5f;
                    draw_list->AddText(font, font_size * 0.75f, ImVec2(textX, textY), 
                                      IM_COL32(255, 255, 255, (int)(255 * alpha)), displayText);
                    
                    // Handle click
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImGui::SetCursorPos(ImVec2(bindX - windowPos.x, bindY - windowPos.y));
                    ImGui::PushID((mod.name + "_bind_btn").c_str());
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    
                    if (ImGui::InvisibleButton("##bindbtn", ImVec2(bindWidth, bindHeight)))
                    {
                        // Start waiting for keybind
                        g_WaitingForKeybind = true;
                        g_KeybindTargetModule = mod.name;
                        g_KeybindTargetCategory = catIdx;
                    }
                    
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
                else
                {
                    // Draw checkbox (always with border, checkmark if enabled)
                    float checkboxX = x + 18.0f;  // 12 * 1.5
                    float checkboxY = settingLineY + 3.0f;  // 2 * 1.5
                    float checkboxSize = 15.0f;  // 10 * 1.5
                    draw_list->AddRect(ImVec2(checkboxX, checkboxY), ImVec2(checkboxX + checkboxSize, checkboxY + checkboxSize), settingColor, 3.0f, 0, 1.5f);  // 2 * 1.5, 1 * 1.5
                    
                    // Draw checkmark if enabled
                    if (settingEnabled)
                    {
                        float checkThickness = 3.0f;  // 2 * 1.5
                        float padding = 4.5f;  // 3 * 1.5
                        ImVec2 p1 = ImVec2(checkboxX + padding, checkboxY + checkboxSize * 0.5f);
                        ImVec2 p2 = ImVec2(checkboxX + checkboxSize * 0.4f, checkboxY + checkboxSize - padding);
                        ImVec2 p3 = ImVec2(checkboxX + checkboxSize - padding, checkboxY + padding);
                        draw_list->AddLine(p1, p2, IM_COL32(255, 255, 255, (int)(255 * alpha)), checkThickness);
                        draw_list->AddLine(p2, p3, IM_COL32(255, 255, 255, (int)(255 * alpha)), checkThickness);
                    }
                    
                    draw_list->AddText(font, font_size * 0.85f, settingPos, settingColor, mod.settings[i].c_str());
                }
            }
        }
        
        currentY += totalHeight;
    }
}

// Check keybinds (call every frame, even when menu is closed)
void CheckKeybinds()
{
    if (g_WaitingForKeybind) return;  // Don't process keybinds while setting one
    
    static bool prevKeyStates[256] = {false};
    
    for (size_t catIdx = 0; catIdx < g_Categories.size(); catIdx++)
    {
        for (auto& mod : g_Categories[catIdx].modules)
        {
            if (mod.keybind != 0)
            {
                bool currentState = (GetAsyncKeyState(mod.keybind) & 0x8000) != 0;
                if (currentState && !prevKeyStates[mod.keybind])
                {
                    // Key just pressed - toggle module
                    mod.enabled = !mod.enabled;
                    
                    // Handle specific modules
                    if (mod.name == "NoClip")
                        CubixDLC::NoClip::GetModule()->SetEnabled(mod.enabled);
                    else if (mod.name == "Invisible")
                        CubixDLC::Invisible::GetModule()->SetEnabled(mod.enabled);
                    else if (mod.name == "ESP")
                    {
                        CubixDLC::ESPEnemy::GetModule()->GetSettings().enabled = mod.enabled;
                        CubixDLC::ESPEnemy::GetModule()->SetEnabled(mod.enabled);
                    }
                    else if (mod.name == "Watermark")
                        GetWatermarkSettings().enabled = mod.enabled;
                    // Escape buttons
                    else if (mod.name == "Door")
                    {
                        CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Door);
                        mod.enabled = false;
                    }
                    else if (mod.name == "Car")
                    {
                        CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Car);
                        mod.enabled = false;
                    }
                    else if (mod.name == "Cellar")
                    {
                        CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Cellar);
                        mod.enabled = false;
                    }
                    else if (mod.name == "Robot")
                    {
                        CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Robot);
                        mod.enabled = false;
                    }
                    
                    PlaySoundResource(mod.enabled ? IDR_SOUND_ON : IDR_SOUND_OFF);
                }
                prevKeyStates[mod.keybind] = currentState;
            }
        }
    }
    
    // Toggle Item Browser with F7
    static bool prevF7 = false;
    bool currentF7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    if (currentF7 && !prevF7)
    {
        g_ShowItemBrowser = !g_ShowItemBrowser;
    }
    prevF7 = currentF7;
}

// Render Item Browser window
void RenderItemBrowser()
{
    if (!g_ShowItemBrowser) return;
    
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float windowWidth = 400.0f;
    float windowHeight = 500.0f;
    
    ImGui::SetNextWindowPos(ImVec2((screenSize.x - windowWidth) * 0.5f, (screenSize.y - windowHeight) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_FirstUseEver);
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.15f, 0.1f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.1f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.2f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.3f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    
    if (ImGui::Begin("Item Browser [F7]", &g_ShowItemBrowser, ImGuiWindowFlags_NoCollapse))
    {
        // Category buttons
        ImGui::Text("Category:");
        ImGui::SameLine();
        
        const CubixDLC::ItemSpawner::ItemCategory categories[] = {
            CubixDLC::ItemSpawner::ItemCategory::All,
            CubixDLC::ItemSpawner::ItemCategory::Tools,
            CubixDLC::ItemSpawner::ItemCategory::Keys,
            CubixDLC::ItemSpawner::ItemCategory::Weapons,
            CubixDLC::ItemSpawner::ItemCategory::Electronics,
            CubixDLC::ItemSpawner::ItemCategory::Puzzle,
            CubixDLC::ItemSpawner::ItemCategory::Food,
            CubixDLC::ItemSpawner::ItemCategory::Parts,
        };
        
        // Category dropdown
        const char* currentCatName = CubixDLC::ItemSpawner::ItemSpawnerModule::GetCategoryName(g_CurrentItemCategory);
        if (ImGui::BeginCombo("##category", currentCatName))
        {
            for (int i = 0; i < 8; i++)
            {
                bool isSelected = (g_CurrentItemCategory == categories[i]);
                const char* catName = CubixDLC::ItemSpawner::ItemSpawnerModule::GetCategoryName(categories[i]);
                if (ImGui::Selectable(catName, isSelected))
                {
                    g_CurrentItemCategory = categories[i];
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        ImGui::Separator();
        
        // Item list
        ImGui::BeginChild("ItemList", ImVec2(0, 0), true);
        
        auto items = CubixDLC::ItemSpawner::ItemSpawnerModule::GetItemsByCategory(g_CurrentItemCategory);
        
        for (const auto* item : items)
        {
            ImGui::PushID(item->name);
            
            // Item button with spawn action
            char buttonLabel[128];
            snprintf(buttonLabel, sizeof(buttonLabel), "%s", item->displayName);
            
            if (ImGui::Button(buttonLabel, ImVec2(-1, 30)))
            {
                // Spawn the item using item name
                bool success = CubixDLC::ItemSpawner::GetModule()->SpawnItem(item->name);
                if (success)
                {
                    LOG_INFO("Picked up: %s", item->displayName);
                }
            }
            
            // Tooltip with item info
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("ID: %s", item->name);
                ImGui::Text("Offset: 0x%X", (unsigned int)item->offset);
                ImGui::Text("Click to spawn in front of you");
                ImGui::EndTooltip();
            }
            
            ImGui::PopID();
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);
}

void RenderHUD()
{
    // Check keybinds even when menu is closed
    CheckKeybinds();
    
    // Render Item Browser (works regardless of menu state)
    RenderItemBrowser();
    
    if (!g_ShowMenu)
        return;
    
    InitializeCategories();
    
    static float time = 0.0f;
    time += ImGui::GetIO().DeltaTime;
    if (time > 100.0f) time = 0.0f;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    ImGui::Begin("ClickGUI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseDown = ImGui::IsMouseDown(0);
    
    // Handle dragging
    static int draggingPanel = -1;
    
    if (ImGui::IsMouseClicked(0))
    {
        // Check if clicking on title bar to start dragging
        for (size_t i = 0; i < g_Categories.size(); i++)
        {
            float titleY = g_Categories[i].posY + TITLE_MARGIN_TOP;
            float titleH = TITLE_HEIGHT;
            
            if (mousePos.x >= g_Categories[i].posX && mousePos.x <= g_Categories[i].posX + PANEL_WIDTH &&
                mousePos.y >= g_Categories[i].posY && mousePos.y <= titleY + titleH)
            {
                draggingPanel = (int)i;
                g_Categories[i].isDragging = true;
                g_Categories[i].dragOffsetX = mousePos.x - g_Categories[i].posX;
                g_Categories[i].dragOffsetY = mousePos.y - g_Categories[i].posY;
                break;
            }
        }
    }
    
    if (!isMouseDown)
    {
        if (draggingPanel >= 0 && draggingPanel < (int)g_Categories.size())
        {
            g_Categories[draggingPanel].isDragging = false;
        }
        draggingPanel = -1;
    }
    
    // Update dragging positions
    static float lastSaveTime = 0.0f;
    if (draggingPanel >= 0 && draggingPanel < (int)g_Categories.size() && isMouseDown)
    {
        g_Categories[draggingPanel].posX = mousePos.x - g_Categories[draggingPanel].dragOffsetX;
        g_Categories[draggingPanel].posY = mousePos.y - g_Categories[draggingPanel].dragOffsetY;
        
        // Clamp to screen bounds
        g_Categories[draggingPanel].posX = (std::max)(0.0f, (std::min)(screenSize.x - PANEL_WIDTH, g_Categories[draggingPanel].posX));
        g_Categories[draggingPanel].posY = (std::max)(0.0f, (std::min)(screenSize.y - PANEL_HEIGHT, g_Categories[draggingPanel].posY));
        
        // Auto-save after dragging (with delay)
        float currentTime = (float)ImGui::GetTime();
        if (currentTime - lastSaveTime > 0.5f) // Save every 0.5 seconds while dragging
        {
            SaveCategoriesToConfig();
            lastSaveTime = currentTime;
        }
    }
    else if (!isMouseDown && draggingPanel >= 0)
    {
        // Save immediately when dragging stops
        SaveCategoriesToConfig();
        lastSaveTime = (float)ImGui::GetTime();
    }
    
    // Render panels
    for (size_t i = 0; i < g_Categories.size(); i++)
    {
        RenderPanel(draw_list, g_Categories[i].posX, g_Categories[i].posY, g_Categories[i], time, i);
    }
    
    // Handle mouse input for modules
    if (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))
    {
        int button = ImGui::IsMouseClicked(0) ? 0 : 1;
        
        for (size_t i = 0; i < g_Categories.size(); i++)
        {
            // Skip if clicking on title bar (for dragging)
            float titleY = g_Categories[i].posY + TITLE_MARGIN_TOP;
            float titleH = TITLE_HEIGHT;
            if (mousePos.y >= g_Categories[i].posY && mousePos.y <= titleY + titleH)
                continue;
            
            if (mousePos.x >= g_Categories[i].posX && mousePos.x <= g_Categories[i].posX + PANEL_WIDTH &&
                mousePos.y >= g_Categories[i].posY && mousePos.y <= g_Categories[i].posY + PANEL_HEIGHT)
            {
                float currentY = g_Categories[i].posY + SCROLL_AREA_Y_OFFSET - g_Categories[i].scrollOffset;
                
                for (auto& mod : g_Categories[i].modules)
                {
                    float prog = g_ExpandProgress[mod.name];
                    float settingsHeight = mod.settings.size() * 27.0f * prog;  // 18 * 1.5
                    float totalHeight = FUNCTION_HEIGHT + settingsHeight;
                    
                    // Check if clicking on module name area
                    if (mousePos.y >= currentY && mousePos.y <= currentY + FUNCTION_HEIGHT &&
                        mousePos.y >= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET &&
                        mousePos.y <= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET + SCROLL_AREA_HEIGHT)
                    {
                        if (button == 0)
                        {
                            bool wasEnabled = mod.enabled;
                            mod.enabled = !mod.enabled;
                            
                            // Handle watermark module
                            if (mod.name == "Watermark")
                            {
                                GetWatermarkSettings().enabled = mod.enabled;
                            }
                            
                            // Handle ESP module
                            if (mod.name == "ESP")
                            {
                                CubixDLC::ESPEnemy::GetModule()->GetSettings().enabled = mod.enabled;
                                CubixDLC::ESPEnemy::GetModule()->SetEnabled(mod.enabled);
                            }
                            
                            // Handle NoClip module
                            if (mod.name == "NoClip")
                            {
                                CubixDLC::NoClip::GetModule()->SetEnabled(mod.enabled);
                            }
                            
                            // Handle Invisible module
                            if (mod.name == "Invisible")
                            {
                                CubixDLC::Invisible::GetModule()->SetEnabled(mod.enabled);
                            }
                            
                            // Handle Escape modules (buttons, not toggles)
                            if (mod.name == "Door")
                            {
                                mod.enabled = false;  // Reset - it's a button, not toggle
                                CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Door);
                            }
                            else if (mod.name == "Car")
                            {
                                mod.enabled = false;
                                CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Car);
                            }
                            else if (mod.name == "Cellar")
                            {
                                mod.enabled = false;
                                CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Cellar);
                            }
                            else if (mod.name == "Robot")
                            {
                                mod.enabled = false;
                                CubixDLC::Escapes::GetModule()->TriggerEscape(CubixDLC::Escapes::EscapeType::Robot);
                            }
                            
                            // Play sound from embedded resources
                            if (mod.enabled && !wasEnabled)
                            {
                                PlaySoundResource(IDR_SOUND_ON);
                            }
                            else if (!mod.enabled && wasEnabled)
                            {
                                PlaySoundResource(IDR_SOUND_OFF);
                            }
                            
                            // Save config after module state change
                            SaveCategoriesToConfig();
                        }
                        else if (button == 1 && !mod.settings.empty())
                        {
                            mod.expanded = !mod.expanded;
                        }
                    }
                    // Check if clicking on settings (if expanded)
                    else if (mod.expanded && settingsHeight > 0.0f && button == 0 &&
                             mousePos.y >= currentY + FUNCTION_HEIGHT &&
                             mousePos.y <= currentY + totalHeight &&
                             mousePos.y >= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET &&
                             mousePos.y <= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET + SCROLL_AREA_HEIGHT)
                    {
                        float settingY = currentY + FUNCTION_HEIGHT;
                        for (size_t j = 0; j < mod.settings.size(); j++)
                        {
                            float settingTop = settingY + j * 27.0f;  // 18 * 1.5
                            float settingBottom = settingTop + 27.0f;  // 18 * 1.5
                            
                            if (mousePos.y >= settingTop && mousePos.y <= settingBottom)
                            {
                                // Toggle setting for watermark module
                                if (mod.name == "Watermark" && mod.settings[j] == "Show FPS")
                                {
                                    GetWatermarkSettings().showFPS = !GetWatermarkSettings().showFPS;
                                    PlaySoundResource(GetWatermarkSettings().showFPS ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    // Save config after setting change
                                    SaveCategoriesToConfig();
                                }
                                // Toggle settings for ESP module
                                else if (mod.name == "ESP")
                                {
                                    auto& espSettings = CubixDLC::ESPEnemy::GetModule()->GetSettings();
                                    
                                    if (mod.settings[j] == "Show Box")
                                    {
                                        espSettings.showBox = !espSettings.showBox;
                                        PlaySoundResource(espSettings.showBox ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    }
                                    else if (mod.settings[j] == "Show Name")
                                    {
                                        espSettings.showName = !espSettings.showName;
                                        PlaySoundResource(espSettings.showName ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    }
                                    else if (mod.settings[j] == "Show Distance")
                                    {
                                        espSettings.showDistance = !espSettings.showDistance;
                                        PlaySoundResource(espSettings.showDistance ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    }
                                    else if (mod.settings[j] == "Show Granny")
                                    {
                                        espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Granny].enabled = 
                                            !espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Granny].enabled;
                                        PlaySoundResource(espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Granny].enabled ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    }
                                    else if (mod.settings[j] == "Show Grandpa")
                                    {
                                        espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Grandpa].enabled = 
                                            !espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Grandpa].enabled;
                                        PlaySoundResource(espSettings.enemyColors[CubixDLC::ESPEnemy::EnemyType::Grandpa].enabled ? IDR_SOUND_ON : IDR_SOUND_OFF);
                                    }
                                    // Color picker settings - handled separately
                                    
                                    // Save config after setting change
                                    SaveCategoriesToConfig();
                                }
                                break;
                            }
                        }
                    }
                    
                    currentY += totalHeight;
                }
                break;
            }
        }
    }
    
    // Handle scroll
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f)
    {
        for (size_t i = 0; i < g_Categories.size(); i++)
        {
            if (mousePos.x >= g_Categories[i].posX && mousePos.x <= g_Categories[i].posX + PANEL_WIDTH &&
                mousePos.y >= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET &&
                mousePos.y <= g_Categories[i].posY + SCROLL_AREA_Y_OFFSET + SCROLL_AREA_HEIGHT)
            {
                g_Categories[i].scrollTarget -= scroll * SCROLL_SPEED;
                break;
            }
        }
    }
    
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
    
    // Keybind setting popup
    if (g_WaitingForKeybind)
    {
        ImVec2 popupSize(300, 100);
        ImVec2 popupPos((screenSize.x - popupSize.x) * 0.5f, (screenSize.y - popupSize.y) * 0.5f);
        
        ImGui::SetNextWindowPos(popupPos);
        ImGui::SetNextWindowSize(popupSize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.95f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        
        ImGui::Begin("Set Keybind", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        
        ImGui::SetCursorPosY(20);
        char text[128];
        snprintf(text, sizeof(text), "Press key for: %s", g_KeybindTargetModule.c_str());
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImGui::SetCursorPosX((popupSize.x - textSize.x) * 0.5f);
        ImGui::Text("%s", text);
        
        ImGui::SetCursorPosY(45);
        ImGui::SetCursorPosX((popupSize.x - 200) * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(ESC to cancel, DEL to clear)");
        
        // Check for key press
        for (int vk = 0x01; vk <= 0xFE; vk++)
        {
            // Skip mouse buttons and some system keys
            if (vk <= 0x06 || vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || 
                vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL ||
                vk == VK_LMENU || vk == VK_RMENU || vk == VK_CAPITAL || vk == VK_NUMLOCK ||
                vk == VK_SCROLL || vk == VK_PAUSE)
                continue;
            
            if (GetAsyncKeyState(vk) & 0x8000)
            {
                if (vk == VK_ESCAPE)
                {
                    // Cancel
                    g_WaitingForKeybind = false;
                    g_KeybindTargetModule = "";
                }
                else if (vk == VK_DELETE && g_KeybindTargetCategory < g_Categories.size())
                {
                    // Clear keybind
                    for (auto& mod : g_Categories[g_KeybindTargetCategory].modules)
                    {
                        if (mod.name == g_KeybindTargetModule)
                        {
                            mod.keybind = 0;
                            break;
                        }
                    }
                    g_WaitingForKeybind = false;
                    g_KeybindTargetModule = "";
                    SaveCategoriesToConfig();
                }
                else if (g_KeybindTargetCategory < g_Categories.size())
                {
                    // Set keybind
                    for (auto& mod : g_Categories[g_KeybindTargetCategory].modules)
                    {
                        if (mod.name == g_KeybindTargetModule)
                        {
                            mod.keybind = vk;
                            LOG_INFO("Set keybind for %s: %s", mod.name.c_str(), GetKeyName(vk));
                            break;
                        }
                    }
                    g_WaitingForKeybind = false;
                    g_KeybindTargetModule = "";
                    SaveCategoriesToConfig();
                }
                break;
            }
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    
    // Render debug console
    RenderDebugConsole();
}

void RenderDebugConsole()
{
    static bool showConsole = false;
    static bool autoScroll = true;
    
    // Toggle console with F1
    if (ImGui::IsKeyPressed(ImGuiKey_F1))
    {
        showConsole = !showConsole;
    }
    
    if (!showConsole)
        return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Debug Console", &showConsole, ImGuiWindowFlags_None))
    {
        // Toolbar
        if (ImGui::Button("Clear"))
        {
            CubixDLC::Logger::Logger::GetInstance().ClearLogMessages();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::SameLine();
        ImGui::Text("Press F1 to toggle");
        
        ImGui::Separator();
        
        // Log display
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        const auto& logMessages = CubixDLC::Logger::Logger::GetInstance().GetLogMessages();
        
        for (const auto& msg : logMessages)
        {
            // Color coding based on log level
            ImU32 color = IM_COL32(255, 255, 255, 255); // Default white
            
            if (msg.find("[ERROR]") != std::string::npos)
                color = IM_COL32(255, 100, 100, 255); // Red
            else if (msg.find("[WARN]") != std::string::npos)
                color = IM_COL32(255, 200, 100, 255); // Orange
            else if (msg.find("[INFO]") != std::string::npos)
                color = IM_COL32(150, 200, 255, 255); // Light blue
            else if (msg.find("[DEBUG]") != std::string::npos)
                color = IM_COL32(150, 150, 150, 255); // Gray
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(msg.c_str());
            ImGui::PopStyleColor();
        }
        
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

