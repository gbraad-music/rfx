/*
 * FX EQ UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_EQ_UI_H
#define FX_EQ_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace EQ {

/**
 * Render 3-band EQ UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* low, float* mid, float* high, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("EQ");

    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##eq", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Low", "##eq_low", low)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Mid", "##eq_mid", mid)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("High", "##eq_high", high)) {
        changed = true;
    }

    return changed;
}

} // namespace EQ
} // namespace FX

#endif // FX_EQ_UI_H
