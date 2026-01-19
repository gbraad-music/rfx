/*
 * FX Stereo Widen UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_STEREO_WIDEN_UI_H
#define FX_STEREO_WIDEN_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace StereoWiden {

/**
 * Render stereo widen effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* width, float* mix, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("STEREO WIDEN");

    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##stereo", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Width", "##stereo_width", width, 0.0f, 2.0f)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Mix", "##stereo_mix", mix)) {
        changed = true;
    }

    return changed;
}

} // namespace StereoWiden
} // namespace FX

#endif // FX_STEREO_WIDEN_UI_H
