/*
 * RFX UI Utilities - Shared UI components for all RFX plugins
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef RFX_UI_UTILS_H
#define RFX_UI_UTILS_H

#include "imgui.h"
#include <cstdio>

namespace RFX {
namespace UI {

// Regroove style colors (matching RegrooveFX)
namespace Colors {
    static const ImVec4 Title        = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);  // Gold
    static const ImVec4 EnabledBtn   = ImVec4(0.81f, 0.10f, 0.22f, 1.0f); // Red (#CF1A37)
    static const ImVec4 DisabledBtn  = ImVec4(0.26f, 0.27f, 0.30f, 1.0f); // Dark gray
    static const ImVec4 FaderHandle  = ImVec4(0.81f, 0.10f, 0.22f, 1.0f); // Red fader
    static const ImVec4 Background   = ImVec4(0.00f, 0.00f, 0.00f, 1.0f); // BLACK (#000000)
    static const ImVec4 FaderBg      = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Dark gray
    static const ImVec4 Text         = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    static const ImVec4 TextDim      = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
}

// Default sizing
namespace Size {
    static const float FaderWidth  = 50.0f;
    static const float FaderHeight = 200.0f;
    static const float ButtonHeight = 30.0f;
    static const float Spacing = 10.0f;
}

/**
 * Apply Regroove style to ImGui (matching RegrooveFX)
 * Call this once at startup for each plugin
 */
inline void setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = Colors::Background;  // BLACK
    colors[ImGuiCol_ChildBg] = Colors::Background;   // BLACK
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg] = Colors::FaderBg;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);  // Grey, not blue!
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);   // Grey, not blue!
    colors[ImGuiCol_Button] = Colors::DisabledBtn;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.36f, 0.37f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.47f, 0.50f, 1.00f);
    colors[ImGuiCol_Text] = Colors::Text;
    colors[ImGuiCol_SliderGrab] = Colors::FaderHandle;  // RED fader handle
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.20f, 0.32f, 1.0f);

    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;   // Rounded fader background
    style.GrabRounding = 3.0f;    // Rounded handle
    style.ItemSpacing = ImVec2(12, 8);
}

/**
 * Render a vertical fader in Regroove style (matching RegrooveFX)
 * Note: This renders ONLY the fader, no label. Labels should be rendered in a separate row.
 */
inline bool renderFader(const char* label, const char* id, float* value,
                       float min = 0.0f, float max = 1.0f,
                       float width = Size::FaderWidth, float height = Size::FaderHeight)
{
    ImGui::BeginGroup();

    // Vertical fader with red handle (matching RegrooveFX)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::FaderBg);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, Colors::FaderHandle);  // Red handle
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.91f, 0.20f, 0.32f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, width - 4.0f);  // Wide handle

    bool changed = ImGui::VSliderFloat(id, ImVec2(width, height), value, min, max, "");

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Label below fader (only if not empty) - keeps it in same vertical group
    if (label && label[0] != '\0') {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
        ImGui::Text("%s", label);
        ImGui::PopStyleColor();
    }

    ImGui::EndGroup();

    return changed;
}

/**
 * Render an enable/disable button in Regroove style (matching RegrooveFX)
 */
inline bool renderEnableButton(const char* id, bool* enabled,
                               float width = Size::FaderWidth, float height = Size::ButtonHeight)
{
    ImVec4 btnColor = *enabled ? Colors::EnabledBtn : Colors::DisabledBtn;
    ImVec4 btnHover = *enabled ? ImVec4(0.91f, 0.20f, 0.32f, 1.0f) : ImVec4(0.36f, 0.37f, 0.40f, 1.0f);
    ImVec4 btnActive = *enabled ? ImVec4(0.71f, 0.05f, 0.17f, 1.0f) : ImVec4(0.46f, 0.47f, 0.50f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);

    bool clicked = ImGui::Button(id, ImVec2(width, height));
    if (clicked) {
        *enabled = !(*enabled);
    }

    ImGui::PopStyleColor(3);
    return clicked;
}

/**
 * Render effect title
 */
inline void renderEffectTitle(const char* title)
{
    ImGui::TextColored(Colors::Title, "%s", title);
    ImGui::Dummy(ImVec2(0, 4.0f));
}

/**
 * Begin an effect group (vertical layout for effect controls)
 */
inline void beginEffectGroup()
{
    ImGui::BeginGroup();
}

/**
 * End an effect group
 */
inline void endEffectGroup()
{
    ImGui::EndGroup();
}

/**
 * Add horizontal spacing between effects
 */
inline void effectSpacing(float spacing = Size::Spacing)
{
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(spacing, 0));
    ImGui::SameLine();
}

} // namespace UI
} // namespace RFX

#endif // RFX_UI_UTILS_H
