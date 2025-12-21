/*
 * FX Limiter UI Component
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_LIMITER_UI_H
#define FX_LIMITER_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Limiter {

/**
 * Render limiter UI (4 parameters)
 * Returns true if any parameter changed
 */
inline bool renderUI(float* threshold, float* release, float* ceiling,
                    float* lookahead, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("LIMITER");

    // Enable button (if enabled parameter provided)
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##limiter", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All faders in horizontal line
    if (RFX::UI::renderFader("Thresh", "##lim_thresh", threshold)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Release", "##lim_release", release)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Ceiling", "##lim_ceiling", ceiling)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Look", "##lim_lookahead", lookahead)) {
        changed = true;
    }

    return changed;
}

} // namespace Limiter
} // namespace FX

#endif // FX_LIMITER_UI_H
