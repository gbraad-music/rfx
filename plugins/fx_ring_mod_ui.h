/*
 * FX Ring Modulator UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_RING_MOD_UI_H
#define FX_RING_MOD_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace RingMod {

/**
 * Render ring modulator effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* frequency, float* mix, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("RING MOD");

    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##ringmod", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Freq", "##ringmod_freq", frequency)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Mix", "##ringmod_mix", mix)) {
        changed = true;
    }

    return changed;
}

} // namespace RingMod
} // namespace FX

#endif // FX_RING_MOD_UI_H
