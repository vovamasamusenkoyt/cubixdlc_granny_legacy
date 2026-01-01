#pragma once

// Watermark module settings
struct WatermarkSettings {
    bool enabled = true;      // Show watermark
    bool showFPS = true;      // Show FPS in watermark
    float posX = 15.0f;      // Position X
    float posY = 15.0f;      // Position Y
    bool isDragging = false; // Is currently being dragged
    float dragOffsetX = 0.0f;
    float dragOffsetY = 0.0f;
};

// Get current watermark settings
WatermarkSettings& GetWatermarkSettings();

// Render watermark (only if enabled)
// menuOpen: true if ClickGUI is open (allows dragging)
void RenderWatermark(bool menuOpen);

