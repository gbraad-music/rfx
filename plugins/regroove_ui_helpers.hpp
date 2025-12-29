#pragma once

/**
 * Regroove UI Helpers - Shared across all VST3 plugins
 *
 * Standard knob configuration matching the Regroove brand identity
 * Uses DearImGuiKnobs with ImGuiKnobVariant_Tick
 *
 * Brand Colors (hardcoded in imgui-knobs.cpp line 235-257):
 * - Outer body: #2A2A2A (42, 42, 42)
 * - Center cap: #555555 (85, 85, 85)
 * - Red tick:   #CF1A37 (207, 26, 55) - SIGNATURE REGROOVE RED
 *
 * Documentation: ../../groovy/STYLE_VST3_UI.md
 * Design System: ../../groovy/STYLE_DESIGN_SYSTEM.md
 *
 * Usage in UI class:
 *   #include "../regroove_ui_helpers.hpp"
 *
 *   // In your ImGuiWidget class:
 *   REGROOVE_KNOB(kParameterLevel, "Level");
 */

#include "DearImGuiKnobs/imgui-knobs.h"

// Regroove standard knob size (matches web UI and VCV Rack)
#define REGROOVE_KNOB_SIZE 45.0f

/**
 * Standard Regroove knob macro for VST3 plugins
 *
 * Parameters:
 *   param_id - Parameter enum value
 *   label    - Display label for the knob
 *
 * This creates a knob with:
 *   - Tick variant (red indicator line)
 *   - 45px size
 *   - No input field (knob-only interaction)
 *   - 8 internal steps for smooth dragging
 *   - 0.0-1.0 range with 0.001 sensitivity
 */
#define REGROOVE_KNOB(param_id, label) \
    do { \
        float value = fUI->fParameters[param_id]; \
        if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f, \
                            "", ImGuiKnobVariant_Tick, REGROOVE_KNOB_SIZE, \
                            ImGuiKnobFlags_NoInput, 8)) { \
            fUI->fParameters[param_id] = value; \
            fUI->setParameterValue(param_id, value); \
        } \
    } while(0)

/**
 * Alternative: Use as a helper function instead of macro
 *
 * Add this to your ImGuiWidget class:
 *
 * private:
 *     void KNOB(uint32_t param, const char* label) {
 *         float value = fUI->fParameters[param];
 *         if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f,
 *                             "", ImGuiKnobVariant_Tick, 45, ImGuiKnobFlags_NoInput, 8)) {
 *             fUI->fParameters[param] = value;
 *             fUI->setParameterValue(param, value);
 *         }
 *     }
 */

// Regroove brand colors for other UI elements
// Matches web UI CSS variables (see /web/index.html lines 17-27)
namespace RegrooveColors {
    // Primary colors (RGB 0-255)
    constexpr int RED_R = 207;
    constexpr int RED_G = 26;
    constexpr int RED_B = 55;

    constexpr int TRACK_R = 42;    // Knob outer body
    constexpr int TRACK_G = 42;
    constexpr int TRACK_B = 42;

    constexpr int CAP_R = 85;      // Knob center cap
    constexpr int CAP_G = 85;
    constexpr int CAP_B = 85;

    constexpr int BG_R = 10;       // Window background (BLACK)
    constexpr int BG_G = 10;
    constexpr int BG_B = 10;

    constexpr int BG_SECONDARY_R = 26;  // Secondary panels
    constexpr int BG_SECONDARY_G = 26;
    constexpr int BG_SECONDARY_B = 26;

    // ImGui colors (0.0-1.0)
    const ImVec4 RED = ImVec4(RED_R/255.0f, RED_G/255.0f, RED_B/255.0f, 1.0f);
    const ImVec4 TRACK = ImVec4(TRACK_R/255.0f, TRACK_G/255.0f, TRACK_B/255.0f, 1.0f);
    const ImVec4 CAP = ImVec4(CAP_R/255.0f, CAP_G/255.0f, CAP_B/255.0f, 1.0f);
    const ImVec4 BG = ImVec4(BG_R/255.0f, BG_G/255.0f, BG_B/255.0f, 1.0f);
    const ImVec4 BG_SECONDARY = ImVec4(BG_SECONDARY_R/255.0f, BG_SECONDARY_G/255.0f, BG_SECONDARY_B/255.0f, 1.0f);

    // Hex values (for documentation/CSS cross-reference)
    // #CF1A37 - Signature Regroove Red (tick line, accents)
    // #2A2A2A - Knob track/outer body
    // #555555 - Knob center cap
    // #0A0A0A - Window background (BLACK)
    // #1A1A1A - Secondary panels
}
