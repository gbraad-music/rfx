/*
 * FX Delay UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_DELAY_UI_H
#define FX_DELAY_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Delay {

/**
 * Render delay effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* time, float* feedback, float* mix, float* enabled = nullptr)
{
    bool changed = false;
    
    RFX::UI::beginEffectGroup();
    RFX::UI::renderEffectTitle("DELAY");
    
    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##delay", &en)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    
    // Time fader
    if (RFX::UI::renderFader("Time", "##delay_time", time)) {
        changed = true;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Feedback fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("FB", "##delay_fb", feedback)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Mix fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Mix", "##delay_mix", mix)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    RFX::UI::endEffectGroup();
    return changed;
}

} // namespace Delay
} // namespace FX

#endif // FX_DELAY_UI_H
