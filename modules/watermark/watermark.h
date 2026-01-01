#pragma once

// Watermark module settings
struct WatermarkSettings {
    bool enabled = true;      // Show watermark
    bool showFPS = true;      // Show FPS in watermark
};

// Get current watermark settings
WatermarkSettings& GetWatermarkSettings();

// Render watermark (only if enabled)
void RenderWatermark();

