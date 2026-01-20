/*
 * FX Resampler UI Component
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_RESAMPLER_UI_H
#define FX_RESAMPLER_UI_H

#include "rfx_ui_utils.h"

namespace FX {
namespace Resampler {

static const char* mode_names[] = {
    "Nearest",
    "Linear",
    "Cubic",
    "Sinc8"
};

/**
 * Render resampler effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* enabled, float* mode, float* rate,
                    float width = RFX::UI::Size::FaderWidth)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;

    RFX::UI::beginEffectGroup();

    // Title
    RFX::UI::renderEffectTitle("RESAMPLER");

    // Enable button
    bool en = *enabled >= 0.5f;
    if (RFX::UI::renderEnableButton("ON##resampler", &en, width)) {
        *enabled = en ? 1.0f : 0.0f;
        changed = true;
    }
    ImGui::Dummy(ImVec2(0, spacing));

    // Mode selector (0-3)
    ImGui::BeginGroup();
    int current_mode = (int)(*mode + 0.5f);
    if (current_mode < 0) current_mode = 0;
    if (current_mode > 3) current_mode = 3;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.81f, 0.10f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.91f, 0.20f, 0.32f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, width - 4.0f);
    if (ImGui::VSliderFloat("##resampler_mode", ImVec2(width, 200.0f), mode, 0.0f, 3.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.0f));
    ImGui::Text("%s", mode_names[current_mode]);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Text("Mode");
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::SameLine(0, spacing);

    // Rate (0.25x to 4.0x)
    if (RFX::UI::renderFader("Rate", "##resampler_rate", rate, 0.25f, 4.0f)) {
        changed = true;
    }

    RFX::UI::endEffectGroup();

    return changed;
}

} // namespace Resampler
} // namespace FX

#endif // FX_RESAMPLER_UI_H
