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
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("COMPRESSOR");

    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##comp", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Thresh", "##comp_thresh", threshold)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Ratio", "##comp_ratio", ratio)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Attack", "##comp_attack", attack)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Release", "##comp_release", release)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Makeup", "##comp_makeup", makeup)) {
        changed = true;
    }

    return changed;
}

} // namespace Compressor
} // namespace FX

#endif // FX_COMPRESSOR_UI_H
