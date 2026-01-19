/*
 * FX Lofi UI Component
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_LOFI_UI_H
#define FX_LOFI_UI_H

#include "../plugins/rfx_ui_utils.h"

namespace FX {
namespace Lofi {

/**
 * Render lofi effect UI
 * Returns true if any parameter changed
 */
inline bool renderUI(float* bit_depth, float* sample_rate_ratio, float* filter_cutoff,
                    float* saturation, float* noise_level, float* wow_flutter_depth,
                    float* wow_flutter_rate, float* enabled = nullptr)
{
    bool changed = false;
    const float spacing = RFX::UI::Size::Spacing;
    const float faderWidth = RFX::UI::Size::FaderWidth;

    // Title
    RFX::UI::renderEffectTitle("LOFI");

    // Enable button
    if (enabled != nullptr) {
        bool en = *enabled >= 0.5f;
        if (RFX::UI::renderEnableButton("ON##lofi", &en, faderWidth)) {
            *enabled = en ? 1.0f : 0.0f;
            changed = true;
        }
        ImGui::Dummy(ImVec2(0, spacing));
    }

    // All 7 faders in one horizontal row

    // Bit Depth - discrete values like AKAI/Amiga (2-bit, 8-bit, 12-bit, 16-bit)
    // Parameter is now an index (0-3), not the actual bit depth
    ImGui::BeginGroup();

    static const char* bit_labels[] = {"2-bit", "8-bit", "12-bit", "16-bit"};
    int current_index = (int)(*bit_depth + 0.5f);  // Round to nearest integer
    if (current_index < 0) current_index = 0;
    if (current_index > 3) current_index = 3;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.81f, 0.10f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.91f, 0.20f, 0.32f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, faderWidth - 4.0f);
    if (ImGui::VSliderFloat("##lofi_bits", ImVec2(faderWidth, 200.0f), bit_depth, 0.0f, 3.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.0f));
    ImGui::Text("%s", bit_labels[current_index]);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Text("Bits");
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::SameLine(0, spacing);

    // Sample Rate - discrete values (Amiga + AKAI vintage rates)
    // Parameter is now an index (0-7), not the actual ratio
    ImGui::BeginGroup();

    static const char* rate_labels[] = {
        "7.5k",    // 0: AKAI S950
        "8.3k",    // 1: Amiga Paula
        "10k",     // 2: AKAI S950
        "15k",     // 3: AKAI S950
        "16.7k",   // 4: Amiga Paula 2x
        "22k",     // 5: AKAI/Standard
        "32k",     // 6: Higher quality
        "48k"      // 7: Clean
    };
    int rate_index = (int)(*sample_rate_ratio + 0.5f);  // Round to nearest integer
    if (rate_index < 0) rate_index = 0;
    if (rate_index > 7) rate_index = 7;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.81f, 0.10f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.91f, 0.20f, 0.32f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, faderWidth - 4.0f);
    if (ImGui::VSliderFloat("##lofi_smprate", ImVec2(faderWidth, 200.0f), sample_rate_ratio, 0.0f, 7.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.0f));
    ImGui::Text("%sHz", rate_labels[rate_index]);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Text("SmpRate");
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Filter", "##lofi_filter", filter_cutoff, 200.0f, 20000.0f)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Sat", "##lofi_sat", saturation, 0.0f, 2.0f)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("Noise", "##lofi_noise", noise_level, 0.0f, 1.0f)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("W/F Dpt", "##lofi_wfd", wow_flutter_depth, 0.0f, 1.0f)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (RFX::UI::renderFader("W/F Rate", "##lofi_wfr", wow_flutter_rate, 0.1f, 10.0f)) {
        changed = true;
    }

    return changed;
}

} // namespace Lofi
} // namespace FX

#endif // FX_LOFI_UI_H
