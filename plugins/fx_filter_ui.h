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
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("FILTER");

    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##filt", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Cutoff", "##filt_cutoff", cutoff)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Reso", "##filt_reso", resonance)) {
        changed = true;
    }

    return changed;
}

} // namespace Filter
} // namespace FX

#endif // FX_FILTER_UI_H
