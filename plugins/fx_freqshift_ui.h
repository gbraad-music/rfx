/*
 * FX Frequency Shifter UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_FREQSHIFT_UI_H
#define FX_FREQSHIFT_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace FreqShift {

/**
 * Render frequency shifter UI (2 parameters)
 * Returns true if any parameter changed
 */
inline bool renderUI(float* freq, float* mix, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("FREQ SHIFT");

    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##freqshift", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Freq", "##fs_freq", freq)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Mix", "##fs_mix", mix)) {
        changed = true;
    }

    return changed;
}

} // namespace FreqShift
} // namespace FX

#endif // FX_FREQSHIFT_UI_H
