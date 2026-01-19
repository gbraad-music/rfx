/*
 * FX Phaser UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_PHASER_UI_H
#define FX_PHASER_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Phaser {

/**
 * Render phaser effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* rate, float* depth, float* feedback, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("PHASER");

    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##phaser", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Rate", "##phaser_rate", rate)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Depth", "##phaser_depth", depth)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Feedback", "##phaser_feedback", feedback)) {
        changed = true;
    }

    return changed;
}

} // namespace Phaser
} // namespace FX

#endif // FX_PHASER_UI_H
