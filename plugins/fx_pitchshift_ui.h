/*
 * FX Pitch Shifter UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_PITCHSHIFT_UI_H
#define FX_PITCHSHIFT_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace PitchShift {

/**
 * Render pitch shifter UI (3 parameters)
 * Returns true if any parameter changed
 */
inline bool renderUI(float* pitch, float* mix, float* formant, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("PITCH SHIFTER");

    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##pitch", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Pitch", "##pitch_pitch", pitch)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Mix", "##pitch_mix", mix)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Formant", "##pitch_formant", formant)) {
        changed = true;
    }

    return changed;
}

} // namespace PitchShift
} // namespace FX

#endif // FX_PITCHSHIFT_UI_H
