/*
 * FX Compressor UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_COMPRESSOR_UI_H
#define FX_COMPRESSOR_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Compressor {

/**
 * Render compressor UI (5 parameters)
 * Returns true if any parameter changed
 */
inline bool renderUI(float* threshold, float* ratio, float* attack, 
                    float* release, float* makeup, float* enabled = nullptr)
{
    bool changed = false;
    
    RFX::UI::beginEffectGroup();
    RFX::UI::renderEffectTitle("COMPRESSOR");
    
    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##comp", &en)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    
    // Threshold fader
    if (RFX::UI::renderFader("Thresh", "##comp_thresh", threshold)) {
        changed = true;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Ratio fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Ratio", "##comp_ratio", ratio)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Attack fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Attack", "##comp_attack", attack)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Release fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Release", "##comp_release", release)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(RFX::UI::Size::Spacing, 0));
    ImGui::SameLine();
    
    // Makeup fader
    ImGui::BeginGroup();
    if (enabled != nullptr) {
        ImGui::Dummy(ImVec2(RFX::UI::Size::FaderWidth, RFX::UI::Size::ButtonHeight));
        ImGui::Dummy(ImVec2(0, RFX::UI::Size::Spacing));
    }
    if (RFX::UI::renderFader("Makeup", "##comp_makeup", makeup)) {
        changed = true;
    }
    ImGui::EndGroup();
    
    RFX::UI::endEffectGroup();
    return changed;
}

} // namespace Compressor
} // namespace FX

#endif // FX_COMPRESSOR_UI_H
