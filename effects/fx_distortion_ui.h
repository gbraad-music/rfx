/*
 * FX Distortion UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_DISTORTION_UI_H
#define FX_DISTORTION_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Distortion {

/**
 * Render distortion effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* drive, float* mix, float* enabled = nullptr)
{
    bool changed = false;
    
    RFX::UI::beginEffectGroup();
    RFX::UI::renderEffectTitle("DISTORTION");
    
    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##dist", &en)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    
    // Drive fader
    if (RFX::UI::renderFader("Drive", "##dist_drive", drive)) {
        changed = true;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Mix fader (aligned with Drive)
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Mix", "##dist_mix", mix)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    RFX::UI::endEffectGroup();
    return changed;
}

} // namespace Distortion
} // namespace FX

#endif // FX_DISTORTION_UI_H
