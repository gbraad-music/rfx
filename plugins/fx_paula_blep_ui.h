/*
 * FX Paula BLEP UI Component
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_PAULA_BLEP_UI_H
#define FX_PAULA_BLEP_UI_H

#include "rfx_ui_utils.h"

namespace FX {
namespace PaulaBlep {

static const char* mode_names[] = {
    "A500",
    "A500+LED",
    "A1200",
    "A1200+LED"
};

/**
 * Render Paula BLEP effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* enabled, float* mode, float* mix,
                    float width = RFX::UI::Size::FaderWidth)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;

    RFX::UI::beginEffectGroup();

    // Title
    RFX::UI::renderEffectTitle("PAULA BLEP");

    // Enable button
    bool en = *enabled >= 0.5f;
    if (RFX::UI::renderEnableButton("ON##paulablep", &en, width)) {
        *enabled = en ? 1.0f : 0.0f;
        changed = true;
    }
    ImGui::Dummy(ImVec2(0, spacing));

    // Mode selector (0-3: A500, A500+LED, A1200, A1200+LED)
    if (RFX::UI::renderFader("Mode", "##paulablep_mode", mode, 0.0f, 3.0f, width)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    // Mix (0.0 to 1.0)
    if (RFX::UI::renderFader("Mix", "##paulablep_mix", mix, 0.0f, 1.0f)) {
        changed = true;
    }

    RFX::UI::endEffectGroup();

    return changed;
}

} // namespace PaulaBlep
} // namespace FX

#endif // FX_PAULA_BLEP_UI_H
