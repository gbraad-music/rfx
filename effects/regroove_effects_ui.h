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

    // Label (only render if not empty)
    if (label && label[0] != '\0') {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
        ImGui::Text("%s", label);
        ImGui::PopStyleColor();
    }

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
    float* compressorAttack, float* compressorRelease, float* compressorMakeup,
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
    const float groupSpacing = 30.0f;  // Extra spacing between effect groups

    // Convert float enables to bool
    bool distEnabled = *distortionEnabled >= 0.5f;
    bool filtEnabled = *filterEnabled >= 0.5f;
    bool eqEn = *eqEnabled >= 0.5f;
    bool compEnabled = *compressorEnabled >= 0.5f;
    bool delEnabled = *delayEnabled >= 0.5f;

    // Row 1: Section titles - positioned at start of each effect group
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Title);

    float titleX = 0.0f;
    ImVec2 startPos = ImGui::GetCursorScreenPos();

    // DISTORTION (fader 1-2)
    ImGui::GetWindowDrawList()->AddText(ImVec2(startPos.x + titleX, startPos.y), ImGui::GetColorU32(ImGuiCol_Text), "DISTORTION");
    titleX += faderWidth * 2 + spacing * 2 + groupSpacing;  // 2 faders + 2 spacings + group gap

    // FILTER (fader 3-4)
    ImGui::GetWindowDrawList()->AddText(ImVec2(startPos.x + titleX, startPos.y), ImGui::GetColorU32(ImGuiCol_Text), "FILTER");
    titleX += faderWidth * 2 + spacing * 2 + groupSpacing;  // 2 faders + 2 spacings + group gap

    // EQ (fader 5-7)
    ImGui::GetWindowDrawList()->AddText(ImVec2(startPos.x + titleX, startPos.y), ImGui::GetColorU32(ImGuiCol_Text), "EQ");
    titleX += faderWidth * 3 + spacing * 3 + groupSpacing;  // 3 faders + 3 spacings + group gap

    // COMPRESSOR (fader 8-12)
    ImGui::GetWindowDrawList()->AddText(ImVec2(startPos.x + titleX, startPos.y), ImGui::GetColorU32(ImGuiCol_Text), "COMPRESSOR");
    titleX += faderWidth * 5 + spacing * 5 + groupSpacing;  // 5 faders + 5 spacings + group gap

    // DELAY (fader 13-15)
    ImGui::GetWindowDrawList()->AddText(ImVec2(startPos.x + titleX, startPos.y), ImGui::GetColorU32(ImGuiCol_Text), "DELAY");

    // Advance cursor past the title row
    ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));

    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 5.0f));

    // Row 2: Parameter labels (Dummy for exact width, text overlay)
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);

    // Helper: label with exact faderWidth
    auto Label = [faderWidth](const char* text) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(faderWidth, ImGui::GetTextLineHeight()));
        ImGui::GetWindowDrawList()->AddText(p, ImGui::GetColorU32(ImGuiCol_Text), text);
    };

    // Distortion - fader 1, 2
    Label("Drive");
    ImGui::SameLine(0, spacing);

    Label("Mix");
    ImGui::SameLine(0, spacing + groupSpacing);

    // Filter - fader 3, 4
    Label("Cutoff");
    ImGui::SameLine(0, spacing);

    Label("Resonance");
    ImGui::SameLine(0, spacing + groupSpacing);

    // EQ - fader 5, 6, 7
    Label("Low");
    ImGui::SameLine(0, spacing);

    Label("Mid");
    ImGui::SameLine(0, spacing);

    Label("High");
    ImGui::SameLine(0, spacing + groupSpacing);

    // Compressor - fader 8, 9, 10, 11, 12
    Label("Threshold");
    ImGui::SameLine(0, spacing);

    Label("Ratio");
    ImGui::SameLine(0, spacing);

    Label("Attack");
    ImGui::SameLine(0, spacing);

    Label("Release");
    ImGui::SameLine(0, spacing);

    Label("Makeup");
    ImGui::SameLine(0, spacing + groupSpacing);

    // Delay - fader 13, 14, 15
    Label("Time");
    ImGui::SameLine(0, spacing);

    Label("Feedback");
    ImGui::SameLine(0, spacing);

    ImGui::Text("Mix");

    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 5.0f));

    // Row 3: Enable buttons using Dummy to match fader positions
    auto EnableBtn = [faderWidth](const char* id, bool* enabled) -> bool {
        ImVec2 savePos = ImGui::GetCursorPos();
        bool ret = renderEnableButton(id, enabled, faderWidth);
        ImGui::SetCursorPos(savePos);
        ImGui::Dummy(ImVec2(faderWidth, ImGui::GetFrameHeight()));
        return ret;
    };

    // DISTORTION enable (above fader 1)
    if (EnableBtn("E##distortion", &distEnabled)) {
        changed = true;
    }
    *distortionEnabled = distEnabled ? 1.0f : 0.0f;
    ImGui::SameLine(0, faderWidth * 1 + spacing * 2 + groupSpacing);  // Skip to next group start

    // FILTER enable (above fader 3)
    if (EnableBtn("E##filter", &filtEnabled)) {
        changed = true;
    }
    *filterEnabled = filtEnabled ? 1.0f : 0.0f;
    ImGui::SameLine(0, faderWidth * 1 + spacing * 2 + groupSpacing);  // Skip to next group start

    // EQ enable (above fader 5)
    if (EnableBtn("E##eq", &eqEn)) {
        changed = true;
    }
    *eqEnabled = eqEn ? 1.0f : 0.0f;
    ImGui::SameLine(0, faderWidth * 2 + spacing * 3 + groupSpacing);  // Skip to next group start

    // COMPRESSOR enable (above fader 8)
    if (EnableBtn("E##compressor", &compEnabled)) {
        changed = true;
    }
    *compressorEnabled = compEnabled ? 1.0f : 0.0f;
    ImGui::SameLine(0, faderWidth * 4 + spacing * 5 + groupSpacing);  // Skip to next group start

    // DELAY enable (above fader 13)
    if (EnableBtn("E##delay", &delEnabled)) {
        changed = true;
    }
    *delayEnabled = delEnabled ? 1.0f : 0.0f;

    ImGui::Dummy(ImVec2(0, 10.0f));

    // Row 4: All 15 faders in ONE horizontal line with group spacing
    // Distortion (2 faders)
    if (renderFader("##dist_drive", "", distortionDrive, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##dist_mix", "", distortionMix, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing + groupSpacing);  // Group spacing after distortion

    // Filter (2 faders)
    if (renderFader("##filt_cutoff", "", filterCutoff, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##filt_res", "", filterResonance, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing + groupSpacing);  // Group spacing after filter

    // EQ (3 faders)
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
    ImGui::SameLine(0, spacing + groupSpacing);  // Group spacing after EQ

    // Compressor (5 faders)
    if (renderFader("##comp_thresh", "", compressorThreshold, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_ratio", "", compressorRatio, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_attack", "", compressorAttack, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_release", "", compressorRelease, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing);

    if (renderFader("##comp_makeup", "", compressorMakeup, faderWidth, faderHeight)) {
        changed = true;
    }
    ImGui::SameLine(0, spacing + groupSpacing);  // Group spacing after compressor

    // Delay (3 faders)
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
