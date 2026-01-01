#include "hud.h"
#include "../deps/imgui/imgui.h"
#include "../hook_render/hook_render.h"
#include "../modules/watermark/watermark.h"
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
    
    Module(const char* n) : name(n), enabled(false), expanded(false) {}
};

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

// Initialize categories
void InitializeCategories()
{
    if (g_CategoriesInitialized) return;
    
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float totalWidth = 5 * (PANEL_WIDTH + PANEL_MARGIN) - PANEL_MARGIN;
    float startX = (screenSize.x - totalWidth) * 0.5f;
    float startY = (screenSize.y - PANEL_HEIGHT) * 0.5f;
    
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
    g_Categories.push_back(movement);
    
    // Render category
    Category render("Render");
    render.posX = startX + 2 * (PANEL_WIDTH + PANEL_MARGIN);
    render.posY = startY;
    render.modules.push_back(Module("ESP"));
    render.modules.push_back(Module("Chams"));
    render.modules.push_back(Module("Wallhack"));
    render.modules.push_back(Module("Tracers"));
    g_Categories.push_back(render);
    
    // Player category
    Category player("Player");
    player.posX = startX + 3 * (PANEL_WIDTH + PANEL_MARGIN);
    player.posY = startY;
    player.modules.push_back(Module("Godmode"));
    player.modules.push_back(Module("Infinite Ammo"));
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
    
    // Update total width for 6 panels
    totalWidth = 6 * (PANEL_WIDTH + PANEL_MARGIN) - PANEL_MARGIN;
    startX = (screenSize.x - totalWidth) * 0.5f;
    
    // Recalculate positions for all categories
    for (size_t i = 0; i < g_Categories.size(); i++)
    {
        g_Categories[i].posX = startX + i * (PANEL_WIDTH + PANEL_MARGIN);
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
void RenderPanel(ImDrawList* draw_list, float x, float y, Category& cat, float)
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
        
        // Module background (if enabled) - purple fill
        ImU32 purpleColor = GetPurpleColor();
        ImU32 moduleBg = mod.enabled ? purpleColor : IM_COL32(198, 198, 198, 30);
        
        if (mod.enabled)
        {
            draw_list->AddRectFilled(ImVec2(x + 6, currentY - 1.5f), ImVec2(x + PANEL_WIDTH - 6, currentY + totalHeight - 1.5f), moduleBg, 6.0f);  // 4 * 1.5, 1 * 1.5
        }
        
        // Module name - white text if enabled, gray if disabled
        ImVec2 textPos = ImVec2(x + 15, currentY + 4.5f);  // 10 * 1.5, 3 * 1.5
        ImU32 textColor = mod.enabled ? IM_COL32(255, 255, 255, 255) : IM_COL32(198, 198, 198, 255);
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
        
        currentY += totalHeight;
    }
}

void RenderHUD()
{
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
    if (draggingPanel >= 0 && draggingPanel < (int)g_Categories.size() && isMouseDown)
    {
        g_Categories[draggingPanel].posX = mousePos.x - g_Categories[draggingPanel].dragOffsetX;
        g_Categories[draggingPanel].posY = mousePos.y - g_Categories[draggingPanel].dragOffsetY;
        
        // Clamp to screen bounds
        g_Categories[draggingPanel].posX = (std::max)(0.0f, (std::min)(screenSize.x - PANEL_WIDTH, g_Categories[draggingPanel].posX));
        g_Categories[draggingPanel].posY = (std::max)(0.0f, (std::min)(screenSize.y - PANEL_HEIGHT, g_Categories[draggingPanel].posY));
    }
    
    // Render panels
    for (size_t i = 0; i < g_Categories.size(); i++)
    {
        RenderPanel(draw_list, g_Categories[i].posX, g_Categories[i].posY, g_Categories[i], time);
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
                            
                            // Play sound from embedded resources
                            if (mod.enabled && !wasEnabled)
                            {
                                PlaySoundResource(IDR_SOUND_ON);
                            }
                            else if (!mod.enabled && wasEnabled)
                            {
                                PlaySoundResource(IDR_SOUND_OFF);
                            }
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
}

