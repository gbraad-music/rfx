/**
 * VU Meter - ImGui Rendering Wrapper
 *
 * Renders VU meter using core logic from common/audio_viz/vu_meter.h
 * This is the ImGui-specific rendering layer
 *
 * Usage in plugin:
 *   // In plugin state:
 *   VUMeter vu_meter;
 *
 *   // In init:
 *   vu_meter_init(&vu_meter, getSampleRate(), VU_MODE_PEAK);
 *
 *   // In audio process:
 *   vu_meter_process(&vu_meter, left_sample, right_sample);
 *
 *   // In UI render:
 *   VU::renderVUMeter(&vu_meter, ImVec2(60, 200), VU::STYLE_VERTICAL);
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef VU_METER_IMGUI_H
#define VU_METER_IMGUI_H

#include "../../common/audio_viz/vu_meter.h"
#include "imgui.h"

namespace VU {

// ============================================================================
// Rendering Styles
// ============================================================================

enum Style {
    STYLE_VERTICAL,     // Traditional vertical meter
    STYLE_HORIZONTAL,   // Horizontal bar
    STYLE_COMBINED,     // Compact retro hardware style (space-saving!)
    STYLE_LED,          // LED segment style
    STYLE_CLASSIC_VU    // Needle-style VU meter (future)
};

enum ColorScheme {
    COLOR_GREEN_YELLOW_RED,  // Standard: green < -18dB, yellow < -6dB, red >= -6dB
    COLOR_MONO_GREEN,        // All green
    COLOR_MONO_BLUE,         // All blue
    COLOR_RETRO              // Amiga/Atari style
};

// ============================================================================
// Color Utilities
// ============================================================================

/**
 * Get color for dB level based on scheme
 */
static inline ImU32 getColorForLevel(float db, ColorScheme scheme = COLOR_GREEN_YELLOW_RED) {
    switch (scheme) {
        case COLOR_GREEN_YELLOW_RED:
            if (db >= -6.0f) return IM_COL32(255, 0, 0, 255);      // Red (hot)
            if (db >= -18.0f) return IM_COL32(255, 200, 0, 255);   // Yellow (warn)
            return IM_COL32(0, 255, 0, 255);                       // Green (good)

        case COLOR_MONO_GREEN:
            return IM_COL32(0, 255, 0, 200);

        case COLOR_MONO_BLUE:
            return IM_COL32(50, 150, 255, 200);

        case COLOR_RETRO:
            return IM_COL32(255, 100, 0, 255); // Amiga orange

        default:
            return IM_COL32(0, 255, 0, 255);
    }
}

/**
 * Get gradient color (interpolate based on level)
 */
static inline ImU32 getGradientColor(float normalized, ColorScheme scheme) {
    if (normalized >= 0.8f) return IM_COL32(255, 0, 0, 255);     // Red
    if (normalized >= 0.6f) {
        // Interpolate yellow to red
        float t = (normalized - 0.6f) / 0.2f;
        return IM_COL32(255, (int)(200 * (1.0f - t)), 0, 255);
    }
    if (normalized >= 0.4f) {
        // Interpolate green to yellow
        float t = (normalized - 0.4f) / 0.2f;
        return IM_COL32((int)(255 * t), 255, 0, 255);
    }
    return IM_COL32(0, 255, 0, 255); // Green
}

// ============================================================================
// Vertical VU Meter
// ============================================================================

/**
 * Render vertical stereo VU meter
 *
 * @param vu VU meter core state
 * @param size Width and height of meter
 * @param scheme Color scheme
 * @param show_peak_hold Show peak hold indicator
 * @param show_db_scale Show dB labels
 */
static inline void renderVertical(VUMeter* vu, ImVec2 size,
                                  ColorScheme scheme = COLOR_GREEN_YELLOW_RED,
                                  bool show_peak_hold = true,
                                  bool show_db_scale = false) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float channel_width = (size.x - 4.0f) / 2.0f; // 2px gap between channels
    const float bar_height = size.y - 20.0f; // Leave space for labels

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       IM_COL32(20, 20, 20, 255));

    // === LEFT CHANNEL ===
    float left_db = vu->peak_left_db;
    float left_norm = vu_meter_get_normalized(left_db);
    float left_height = left_norm * bar_height;

    // Draw bar with gradient
    if (left_height > 1.0f) {
        ImVec2 bar_start(pos.x + 2, pos.y + bar_height);
        ImVec2 bar_end(pos.x + 2 + channel_width, pos.y + bar_height - left_height);

        // Draw in segments for gradient effect
        for (float y = 0; y < left_height; y += 2.0f) {
            float segment_norm = (bar_height - y) / bar_height;
            ImU32 color = getGradientColor(segment_norm, scheme);

            draw->AddRectFilled(
                ImVec2(bar_start.x, bar_start.y - y - 2.0f),
                ImVec2(bar_end.x, bar_start.y - y),
                color
            );
        }
    }

    // Peak hold line (left)
    if (show_peak_hold && vu->peak_hold_left > 0.0f) {
        float hold_norm = vu_meter_get_normalized(vu->peak_hold_left_db);
        float hold_y = pos.y + bar_height - (hold_norm * bar_height);

        draw->AddLine(
            ImVec2(pos.x + 2, hold_y),
            ImVec2(pos.x + 2 + channel_width, hold_y),
            IM_COL32(255, 255, 255, 255),
            2.0f
        );
    }

    // === RIGHT CHANNEL ===
    float right_db = vu->peak_right_db;
    float right_norm = vu_meter_get_normalized(right_db);
    float right_height = right_norm * bar_height;

    if (right_height > 1.0f) {
        ImVec2 bar_start(pos.x + channel_width + 4, pos.y + bar_height);
        ImVec2 bar_end(pos.x + size.x - 2, pos.y + bar_height - right_height);

        for (float y = 0; y < right_height; y += 2.0f) {
            float segment_norm = (bar_height - y) / bar_height;
            ImU32 color = getGradientColor(segment_norm, scheme);

            draw->AddRectFilled(
                ImVec2(bar_start.x, bar_start.y - y - 2.0f),
                ImVec2(bar_end.x, bar_start.y - y),
                color
            );
        }
    }

    // Peak hold line (right)
    if (show_peak_hold && vu->peak_hold_right > 0.0f) {
        float hold_norm = vu_meter_get_normalized(vu->peak_hold_right_db);
        float hold_y = pos.y + bar_height - (hold_norm * bar_height);

        draw->AddLine(
            ImVec2(pos.x + channel_width + 4, hold_y),
            ImVec2(pos.x + size.x - 2, hold_y),
            IM_COL32(255, 255, 255, 255),
            2.0f
        );
    }

    // === dB SCALE (optional) ===
    if (show_db_scale) {
        const float db_marks[] = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

        for (int i = 0; i < 5; i++) {
            float norm = vu_meter_get_normalized(db_marks[i]);
            float y = pos.y + bar_height - (norm * bar_height);

            // Tick mark
            draw->AddLine(
                ImVec2(pos.x, y),
                ImVec2(pos.x + 4, y),
                IM_COL32(100, 100, 100, 255)
            );

            // Label
            char label[8];
            snprintf(label, sizeof(label), "%.0f", db_marks[i]);
            draw->AddText(ImVec2(pos.x + size.x + 5, y - 7), IM_COL32(150, 150, 150, 255), label);
        }

        ImGui::PopStyleColor();
    }

    // === CHANNEL LABELS ===
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 5, pos.y + bar_height + 5));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Text("L");
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(pos.x + channel_width + 10, pos.y + bar_height + 5));
    ImGui::Text("R");
    ImGui::PopStyleColor();

    // Reserve space
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y));
    ImGui::Dummy(size);
}

// ============================================================================
// Horizontal VU Meter
// ============================================================================

/**
 * Render horizontal stereo VU meter (stacked)
 */
static inline void renderHorizontal(VUMeter* vu, ImVec2 size,
                                   ColorScheme scheme = COLOR_GREEN_YELLOW_RED,
                                   bool show_peak_hold = true) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float channel_height = (size.y - 4.0f) / 2.0f;
    const float bar_width = size.x - 40.0f; // Leave space for labels

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       IM_COL32(20, 20, 20, 255));

    // === LEFT CHANNEL (top) ===
    float left_norm = vu_meter_get_normalized(vu->peak_left_db);
    float left_width = left_norm * bar_width;

    if (left_width > 1.0f) {
        ImVec2 bar_start(pos.x + 20, pos.y + 2);
        ImVec2 bar_end(pos.x + 20 + left_width, pos.y + 2 + channel_height);

        for (float x = 0; x < left_width; x += 2.0f) {
            float segment_norm = x / bar_width;
            ImU32 color = getGradientColor(segment_norm, scheme);

            draw->AddRectFilled(
                ImVec2(bar_start.x + x, bar_start.y),
                ImVec2(bar_start.x + x + 2.0f, bar_end.y),
                color
            );
        }
    }

    // === RIGHT CHANNEL (bottom) ===
    float right_norm = vu_meter_get_normalized(vu->peak_right_db);
    float right_width = right_norm * bar_width;

    if (right_width > 1.0f) {
        ImVec2 bar_start(pos.x + 20, pos.y + channel_height + 4);
        ImVec2 bar_end(pos.x + 20 + right_width, pos.y + size.y - 2);

        for (float x = 0; x < right_width; x += 2.0f) {
            float segment_norm = x / bar_width;
            ImU32 color = getGradientColor(segment_norm, scheme);

            draw->AddRectFilled(
                ImVec2(bar_start.x + x, bar_start.y),
                ImVec2(bar_start.x + x + 2.0f, bar_end.y),
                color
            );
        }
    }

    // Labels
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 2, pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Text("L");
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 2, pos.y + channel_height + 8));
    ImGui::Text("R");
    ImGui::PopStyleColor();

    // Reserve space
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y));
    ImGui::Dummy(size);
}

// ============================================================================
// Combined/Compact VU Meter (Retro Hardware Style)
// ============================================================================

/**
 * Render compact combined stereo meter (space-saving retro look)
 * Both channels in single unit with center gap
 */
static inline void renderCombined(VUMeter* vu, ImVec2 size,
                                 ColorScheme scheme = COLOR_RETRO,
                                 bool show_peak_hold = true) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float bar_height = size.y - 15.0f;
    const float center_gap = 4.0f;
    const float half_width = (size.x - center_gap) / 2.0f;

    // === BACKGROUND (hardware chassis look) ===
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                       IM_COL32(25, 25, 25, 255), 3.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                  IM_COL32(60, 60, 60, 255), 3.0f, 0, 1.5f);

    // === LEFT CHANNEL (grows from center to left) ===
    float left_norm = vu_meter_get_normalized(vu->peak_left_db);
    float left_width = left_norm * half_width;

    if (left_width > 1.0f) {
        ImVec2 bar_start(pos.x + half_width - left_width + 2, pos.y + 5);
        ImVec2 bar_end(pos.x + half_width, pos.y + 5 + bar_height);

        // Draw segments (retro LED-style)
        const int num_segments = 20;
        const float segment_height = bar_height / num_segments;

        for (int i = 0; i < num_segments; i++) {
            float segment_norm = (float)i / num_segments;
            if (segment_norm <= left_norm) {
                ImU32 color = getGradientColor(segment_norm, scheme);

                draw->AddRectFilled(
                    ImVec2(bar_start.x, bar_end.y - (i + 1) * segment_height + 1),
                    ImVec2(bar_end.x - 1, bar_end.y - i * segment_height - 1),
                    color,
                    1.0f
                );
            }
        }
    }

    // === RIGHT CHANNEL (grows from center to right) ===
    float right_norm = vu_meter_get_normalized(vu->peak_right_db);
    float right_width = right_norm * half_width;

    if (right_width > 1.0f) {
        ImVec2 bar_start(pos.x + half_width + center_gap, pos.y + 5);
        ImVec2 bar_end(pos.x + half_width + center_gap + right_width - 2, pos.y + 5 + bar_height);

        const int num_segments = 20;
        const float segment_height = bar_height / num_segments;

        for (int i = 0; i < num_segments; i++) {
            float segment_norm = (float)i / num_segments;
            if (segment_norm <= right_norm) {
                ImU32 color = getGradientColor(segment_norm, scheme);

                draw->AddRectFilled(
                    ImVec2(bar_start.x + 1, bar_end.y - (i + 1) * segment_height + 1),
                    ImVec2(bar_end.x, bar_end.y - i * segment_height - 1),
                    color,
                    1.0f
                );
            }
        }
    }

    // === CENTER DIVIDER ===
    draw->AddLine(
        ImVec2(pos.x + half_width, pos.y + 5),
        ImVec2(pos.x + half_width, pos.y + 5 + bar_height),
        IM_COL32(80, 80, 80, 255),
        2.0f
    );

    // === PEAK HOLD INDICATORS ===
    if (show_peak_hold) {
        // Left peak hold (horizontal line from center)
        if (vu->peak_hold_left > 0.0f) {
            float hold_norm = vu_meter_get_normalized(vu->peak_hold_left_db);
            float hold_width = hold_norm * half_width;
            float hold_x = pos.x + half_width - hold_width;

            draw->AddLine(
                ImVec2(hold_x, pos.y + 5),
                ImVec2(hold_x, pos.y + 5 + bar_height),
                IM_COL32(255, 255, 255, 200),
                1.5f
            );
        }

        // Right peak hold
        if (vu->peak_hold_right > 0.0f) {
            float hold_norm = vu_meter_get_normalized(vu->peak_hold_right_db);
            float hold_width = hold_norm * half_width;
            float hold_x = pos.x + half_width + center_gap + hold_width;

            draw->AddLine(
                ImVec2(hold_x, pos.y + 5),
                ImVec2(hold_x, pos.y + 5 + bar_height),
                IM_COL32(255, 255, 255, 200),
                1.5f
            );
        }
    }

    // === CHANNEL LABELS (compact) ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 5, pos.y + bar_height + 8));
    ImGui::Text("L");
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 12, pos.y + bar_height + 8));
    ImGui::Text("R");
    ImGui::PopStyleColor();

    // Reserve space
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y));
    ImGui::Dummy(size);
}

// ============================================================================
// Main Render Function (auto-selects style)
// ============================================================================

/**
 * Render VU meter with auto-selected style
 */
static inline void renderVUMeter(VUMeter* vu, ImVec2 size,
                                Style style = STYLE_VERTICAL,
                                ColorScheme scheme = COLOR_GREEN_YELLOW_RED,
                                bool show_peak_hold = true,
                                bool show_db_scale = false) {
    switch (style) {
        case STYLE_VERTICAL:
            renderVertical(vu, size, scheme, show_peak_hold, show_db_scale);
            break;

        case STYLE_HORIZONTAL:
            renderHorizontal(vu, size, scheme, show_peak_hold);
            break;

        case STYLE_LED:
        case STYLE_CLASSIC_VU:
            // TODO: Implement LED and classic needle styles
            renderVertical(vu, size, scheme, show_peak_hold, show_db_scale);
            break;
    }
}

} // namespace VU

#endif // VU_METER_IMGUI_H
