/*
 * RegrooveFX UI - Framework-agnostic ImGui rendering
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 *
 * Pure ImGui rendering functions that work with ANY ImGui context:
 * - DPF plugins (with DearImGui/OpenGL)
 * - SDL applications (../regroove, ../samplecrate)
 * - Any other ImGui integration
 *
 * Usage:
 *   1. In your ImGui render loop, call RegrooveFX::UI::render()
 *   2. Pass pointers to your parameter values
 *   3. Check return value to see if any parameter changed
 */

#ifndef REGROOVE_EFFECTS_UI_H
#define REGROOVE_EFFECTS_UI_H

#include "imgui.h"

namespace RegrooveFX {
namespace UI {

// Color scheme matching Regroove brand
namespace Colors {
    static const ImVec4 Title        = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);  // Gold
    static const ImVec4 EnabledBtn   = ImVec4(0.81f, 0.10f, 0.22f, 1.0f); // Red (#CF1A37)
    static const ImVec4 DisabledBtn  = ImVec4(0.26f, 0.27f, 0.30f, 1.0f); // Dark gray
    static const ImVec4 FaderHandle  = ImVec4(0.81f, 0.10f, 0.22f, 1.0f); // Red fader
    static const ImVec4 Background   = ImVec4(0.00f, 0.00f, 0.00f, 1.0f); // BLACK (#000000)
    static const ImVec4 FaderBg      = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Dark gray
    static const ImVec4 Text         = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    static const ImVec4 TextDim      = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
}

// Apply Regroove style to ImGui (call once at startup)
inline void setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = Colors::Background;  // BLACK
    colors[ImGuiCol_ChildBg] = Colors::Background;   // BLACK
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg] = Colors::FaderBg;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Button] = Colors::DisabledBtn;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.36f, 0.37f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.47f, 0.50f, 1.00f);
    colors[ImGuiCol_Text] = Colors::Text;
    colors[ImGuiCol_SliderGrab] = Colors::FaderHandle;  // RED fader handle
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.20f, 0.32f, 1.00f);

    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ItemSpacing = ImVec2(12, 8);
}

// Render a fader (vertical slider matching /tmp/faders.png design)
inline bool renderFader(const char* id, const char* label, float* value,
                        float width = 50.0f, float height = 200.0f,
                        float min = 0.0f, float max = 1.0f) {
    bool changed = false;

    // Vertical fader with red handle (matching /tmp/faders.png)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::FaderBg);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, Colors::FaderHandle);  // Red handle
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.91f, 0.20f, 0.32f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, width - 4.0f);  // Wide handle like faders.png

    if (ImGui::VSliderFloat(id, ImVec2(width, height), value, min, max, "")) {
        changed = true;
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Label
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();

    return changed;
}

// Render enable button
inline bool renderEnableButton(const char* id, bool* enabled, float width = 65.0f) {
    bool clicked = false;

    ImVec4 btnColor = *enabled ? Colors::EnabledBtn : Colors::DisabledBtn;
    ImVec4 btnHover = *enabled ? ImVec4(0.91f, 0.20f, 0.32f, 1.0f) : ImVec4(0.36f, 0.37f, 0.40f, 1.0f);
    ImVec4 btnActive = *enabled ? ImVec4(0.71f, 0.05f, 0.17f, 1.0f) : ImVec4(0.46f, 0.47f, 0.50f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);

    if (ImGui::Button(id, ImVec2(width, 28.0f))) {
        *enabled = !(*enabled);
        clicked = true;
    }

    ImGui::PopStyleColor(3);
    return clicked;
}

// Render effect section (2 or 3 knobs with enable button)
inline bool renderEffectSection(const char* name,
                               bool* enabled,
                               float* param1, const char* label1,
                               float* param2, const char* label2,
                               float* param3 = nullptr, const char* label3 = nullptr,
                               float knobSize = 65.0f, float spacing = 20.0f) {
    bool changed = false;

    ImGui::BeginGroup();

    // Section title
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Title);
    ImGui::Text("%s", name);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8.0f));

    // Enable button
    char btnId[64];
    snprintf(btnId, sizeof(btnId), " E ##enable_%s", name);
    if (renderEnableButton(btnId, enabled, knobSize)) {
        changed = true;
    }

    ImGui::Dummy(ImVec2(0, spacing));

    // Fader 1
    char fader1Id[64];
    snprintf(fader1Id, sizeof(fader1Id), "##%s_1", name);
    if (renderFader(fader1Id, label1, param1, knobSize)) {
        changed = true;
    }

    // Fader 2
    ImGui::SameLine(0, spacing);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(knobSize, 28.0f + spacing));

    char fader2Id[64];
    snprintf(fader2Id, sizeof(fader2Id), "##%s_2", name);
    if (renderFader(fader2Id, label2, param2, knobSize)) {
        changed = true;
    }
    ImGui::EndGroup();

    // Optional fader 3
    if (param3 != nullptr && label3 != nullptr) {
        ImGui::SameLine(0, spacing);
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(knobSize, 28.0f + spacing));

        char fader3Id[64];
        snprintf(fader3Id, sizeof(fader3Id), "##%s_3", name);
        if (renderFader(fader3Id, label3, param3, knobSize)) {
            changed = true;
        }
        ImGui::EndGroup();
    }

    ImGui::EndGroup();
    return changed;
}

// Main render function - call this from your ImGui render loop
// Parameters: all 20 effect parameters as normalized floats (0.0-1.0)
// Returns: true if any parameter changed
inline bool render(
    // Distortion
    float* distortionEnabled, float* distortionDrive, float* distortionMix,
    // Filter
    float* filterEnabled, float* filterCutoff, float* filterResonance,
    // EQ
    float* eqEnabled, float* eqLow, float* eqMid, float* eqHigh,
    // Compressor
    float* compressorEnabled, float* compressorThreshold, float* compressorRatio,
    // Delay
    float* delayEnabled, float* delayTime, float* delayFeedback, float* delayMix,
    // Layout options
    float windowWidth = 1000.0f,
    float windowHeight = 450.0f,
    bool showTitle = true)
{
    bool changed = false;
    const float faderWidth = 60.0f;
    const float faderHeight = 220.0f;
    const float spacing = 15.0f;

    // Convert float enables to bool
    bool distEnabled = *distortionEnabled >= 0.5f;
    bool filtEnabled = *filterEnabled >= 0.5f;
    bool eqEn = *eqEnabled >= 0.5f;
    bool compEnabled = *compressorEnabled >= 0.5f;
    bool delEnabled = *delayEnabled >= 0.5f;

    // Row 1: Section titles
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Title);

    // DISTORTION spans 2 faders
    ImGui::Text("DISTORTION");
    ImGui::SameLine(0, faderWidth + spacing);

    // FILTER spans 2 faders
    ImGui::Text("FILTER");
    ImGui::SameLine(0, faderWidth + spacing);

    // EQ spans 3 faders
    ImGui::Text("EQ");
    ImGui::SameLine(0, faderWidth * 2 + spacing * 2);

    // COMPRESSOR spans 2 faders
    ImGui::Text("COMPRESSOR");
    ImGui::SameLine(0, faderWidth + spacing);

    // DELAY spans 3 faders
    ImGui::Text("DELAY");

    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 5.0f));

    // Row 2: Parameter labels
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);

    ImGui::Text("Drive"); ImGui::SameLine(0, spacing);
    ImGui::Text("Mix"); ImGui::SameLine(0, spacing);
    ImGui::Text("Cutoff"); ImGui::SameLine(0, spacing);
    ImGui::Text("Resonance"); ImGui::SameLine(0, spacing);
    ImGui::Text("Low"); ImGui::SameLine(0, spacing);
    ImGui::Text("Mid"); ImGui::SameLine(0, spacing);
    ImGui::Text("High"); ImGui::SameLine(0, spacing);
    ImGui::Text("Threshold"); ImGui::SameLine(0, spacing);
    ImGui::Text("Ratio"); ImGui::SameLine(0, spacing);
    ImGui::Text("Time"); ImGui::SameLine(0, spacing);
    ImGui::Text("Feedback"); ImGui::SameLine(0, spacing);
    ImGui::Text("Mix");

    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 5.0f));

    // Row 3: Enable buttons (one per section, aligned with first fader)
    if (renderEnableButton("E##distortion", &distEnabled, faderWidth)) {
        changed = true;
    }
    *distortionEnabled = distEnabled ? 1.0f : 0.0f;

    ImGui::SameLine(0, faderWidth + spacing);
    if (renderEnableButton("E##filter", &filtEnabled, faderWidth)) {
        changed = true;
    }
    *filterEnabled = filtEnabled ? 1.0f : 0.0f;

    ImGui::SameLine(0, faderWidth + spacing);
    if (renderEnableButton("E##eq", &eqEn, faderWidth)) {
        changed = true;
    }
    *eqEnabled = eqEn ? 1.0f : 0.0f;

    ImGui::SameLine(0, faderWidth * 2 + spacing * 2);
    if (renderEnableButton("E##compressor", &compEnabled, faderWidth)) {
        changed = true;
    }
    *compressorEnabled = compEnabled ? 1.0f : 0.0f;

    ImGui::SameLine(0, faderWidth + spacing);
    if (renderEnableButton("E##delay", &delEnabled, faderWidth)) {
        changed = true;
    }
    *delayEnabled = delEnabled ? 1.0f : 0.0f;

    ImGui::Dummy(ImVec2(0, 10.0f));

    // Row 4: All 12 faders in ONE horizontal line
    if (renderFader("##dist_drive", "", distortionDrive, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##dist_mix", "", distortionMix, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##filt_cutoff", "", filterCutoff, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##filt_res", "", filterResonance, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##eq_low", "", eqLow, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##eq_mid", "", eqMid, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##eq_high", "", eqHigh, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_thresh", "", compressorThreshold, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_ratio", "", compressorRatio, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##delay_time", "", delayTime, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##delay_fb", "", delayFeedback, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##delay_mix", "", delayMix, faderWidth, faderHeight)) {
        changed = true;
    }

    return changed;
}

} // namespace UI
} // namespace RegrooveFX

#endif // REGROOVE_EFFECTS_UI_H
