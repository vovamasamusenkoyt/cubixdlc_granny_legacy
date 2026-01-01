#include "watermark.h"
#include "../../deps/imgui/imgui.h"
#include <cmath>
#include <float.h>
#include <stdio.h>

// Watermark settings
static WatermarkSettings g_WatermarkSettings;

WatermarkSettings& GetWatermarkSettings()
{
    return g_WatermarkSettings;
}

void RenderWatermark()
{
    if (!g_WatermarkSettings.enabled)
        return;

    static float time = 0.0f;
    time += ImGui::GetIO().DeltaTime;
    if (time > 100.0f) time = 0.0f; // Reset to avoid overflow

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    const float padding = 12.0f;
    const float height = 30.0f;
    const float x_pos = 15.0f;
    const float y_pos = 15.0f;

    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();

    float text1_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "cubixDLC").x;
    
    // Build watermark text
    char watermark_text[64];
    if (g_WatermarkSettings.showFPS)
    {
        float fps = ImGui::GetIO().Framerate;
        _snprintf_s(watermark_text, sizeof(watermark_text), _TRUNCATE, "| 0.1v | dll | %.0f FPS", fps);
    }
    else
    {
        strcpy_s(watermark_text, sizeof(watermark_text), "| 0.1v | dll");
    }
    
    // Calculate width with max FPS value for consistency
    float text2_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "| 0.1v | dll | 9999 FPS").x;
    if (!g_WatermarkSettings.showFPS)
    {
        text2_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "| 0.1v | dll").x;
    }

    float total_content_width = text1_width + 5.0f + text2_width;
    float actual_width = total_content_width + padding * 2;

    ImVec2 bg_min = ImVec2(x_pos, y_pos);
    ImVec2 bg_max = ImVec2(x_pos + actual_width, y_pos + height);

    ImU32 bg_color = IM_COL32(5, 5, 5, 217);
    draw_list->AddRectFilled(bg_min, bg_max, bg_color, 6.0f);

    float border_r = 0.3f + 0.2f * sinf(time * 2.0f);
    float border_g = 0.5f + 0.3f * sinf(time * 2.0f + 1.0f);
    float border_b = 0.9f;
    border_r = (border_r < 0.0f) ? 0.0f : (border_r > 1.0f) ? 1.0f : border_r;
    border_g = (border_g < 0.0f) ? 0.0f : (border_g > 1.0f) ? 1.0f : border_g;
    ImU32 border_color = IM_COL32((int)(border_r * 255), (int)(border_g * 255), (int)(border_b * 255), 230);
    draw_list->AddRect(bg_min, bg_max, border_color, 6.0f, 0, 2.0f);

    float text_y = y_pos + (height - font_size) * 0.5f;
    ImVec2 text_pos = ImVec2(x_pos + padding, text_y);
    ImVec2 text2_pos = ImVec2(text_pos.x + text1_width + 5.0f, text_y);

    ImVec2 shadow_offset = ImVec2(1.0f, 1.0f);
    draw_list->AddText(font, font_size, ImVec2(text_pos.x + shadow_offset.x, text_pos.y + shadow_offset.y), IM_COL32(0, 0, 0, 180), "cubixDLC");
    draw_list->AddText(font, font_size, ImVec2(text2_pos.x + shadow_offset.x, text2_pos.y + shadow_offset.y), IM_COL32(0, 0, 0, 180), watermark_text);

    float r1 = 0.4f + 0.3f * sinf(time * 1.5f);
    float g1 = 0.6f + 0.3f * sinf(time * 1.5f + 2.0f);
    float b1 = 1.0f;
    float r2 = 0.8f + 0.2f * sinf(time * 1.5f + 1.0f);
    float g2 = 0.9f + 0.1f * sinf(time * 1.5f + 3.0f);
    float b2 = 1.0f;

    r1 = (r1 < 0.0f) ? 0.0f : (r1 > 1.0f) ? 1.0f : r1;
    g1 = (g1 < 0.0f) ? 0.0f : (g1 > 1.0f) ? 1.0f : g1;
    r2 = (r2 < 0.0f) ? 0.0f : (r2 > 1.0f) ? 1.0f : r2;
    g2 = (g2 < 0.0f) ? 0.0f : (g2 > 1.0f) ? 1.0f : g2;

    ImU32 color1 = IM_COL32((int)(r1 * 255), (int)(g1 * 255), (int)(b1 * 255), 255);
    ImU32 color2 = IM_COL32((int)(r2 * 255), (int)(g2 * 255), (int)(b2 * 255), 200);

    draw_list->AddText(font, font_size, text_pos, color1, "cubixDLC");
    draw_list->AddText(font, font_size, text2_pos, color2, watermark_text);

    float dot_alpha = 0.5f + 0.5f * sinf(time * 3.0f);
    dot_alpha = (dot_alpha < 0.0f) ? 0.0f : (dot_alpha > 1.0f) ? 1.0f : dot_alpha;
    ImU32 dot_color = IM_COL32(100, 200, 255, (int)(dot_alpha * 255));
    draw_list->AddCircleFilled(ImVec2(bg_max.x - 10.0f, bg_max.y - 10.0f), 3.0f, dot_color);
}

