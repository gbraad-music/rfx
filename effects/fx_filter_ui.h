/*
 * FX Filter UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_FILTER_UI_H
#define FX_FILTER_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Filter {

/**
 * Render filter effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* cutoff, float* resonance, float* enabled = nullptr)
{
    bool changed = false;
    
    RFX::UI::beginEffectGroup();
    RFX::UI::renderEffectTitle("FILTER");
    
    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##filt", &en)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    
    // Cutoff fader
    if (RFX::UI::renderFader("Cutoff", "##filt_cutoff", cutoff)) {
        changed = true;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Resonance fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Reso", "##filt_reso", resonance)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    RFX::UI::endEffectGroup();
    return changed;
}

} // namespace Filter
} // namespace FX

#endif // FX_FILTER_UI_H
