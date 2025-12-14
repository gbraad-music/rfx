/*
 * FX EQ UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_EQ_UI_H
#define FX_EQ_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace EQ {

/**
 * Render 3-band EQ UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* low, float* mid, float* high, float* enabled = nullptr)
{
    bool changed = false;
    
    RFX::UI::beginEffectGroup();
    RFX::UI::renderEffectTitle("EQ");
    
    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##eq", &en)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    
    // Low fader
    if (RFX::UI::renderFader("Low", "##eq_low", low)) {
        changed = true;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Mid fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Mid", "##eq_mid", mid)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // High fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("High", "##eq_high", high)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    RFX::UI::endEffectGroup();
    return changed;
}

} // namespace EQ
} // namespace FX

#endif // FX_EQ_UI_H
