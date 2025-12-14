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

// Regroove style colors
namespace Colors {
    static const ImVec4 Title       = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);  // Gold
    static const ImVec4 EnabledBtn  = ImVec4(0.70f, 0.60f, 0.20f, 1.0f); // Active gold
    static const ImVec4 DisabledBtn = ImVec4(0.26f, 0.27f, 0.30f, 1.0f); // Dark gray
    static const ImVec4 Fader       = ImVec4(0.85f, 0.70f, 0.25f, 1.0f); // Fader accent
    static const ImVec4 Background  = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Dark background
}

// Default sizing
namespace Size {
    static const float FaderWidth  = 50.0f;
    static const float FaderHeight = 200.0f;
    static const float ButtonHeight = 30.0f;
    static const float Spacing = 10.0f;
}

/**
 * Render a vertical fader in Regroove style
 */
inline bool renderFader(const char* label, const char* id, float* value, 
                       float min = 0.0f, float max = 1.0f,
                       float width = Size::FaderWidth, float height = Size::FaderHeight)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::Background);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, Colors::Fader);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.85f, 0.35f, 1.0f));
    
    bool changed = ImGui::VSliderFloat(id, ImVec2(width, height), value, min, max, "");
    
    ImGui::PopStyleColor(5);
    
    // Label below fader
    ImGui::Text("%s", label);
    
    return changed;
}

/**
 * Render an enable/disable button in Regroove style
 */
inline bool renderEnableButton(const char* id, bool* enabled,
                               float width = Size::FaderWidth, float height = Size::ButtonHeight)
{
    ImVec4 btnColor = *enabled ? Colors::EnabledBtn : Colors::DisabledBtn;
    ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.70f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.80f, 0.40f, 1.0f));
    
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
