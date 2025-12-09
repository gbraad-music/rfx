/*
 * RegrooveFX Plugin UI
 * Copyright (C) 2024
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 */

#include "DistrhoUI.hpp"
#include "imgui.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class RegrooveFXUI : public UI
{
public:
    RegrooveFXUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
    }

protected:
    // -------------------------------------------------------------------
    // DSP/Plugin Callbacks

    void parameterChanged(uint32_t index, float value) override
    {
        // Store parameter values for UI display
        fParameters[index] = value;
        repaint();
    }

    // -------------------------------------------------------------------
    // Widget Callbacks

    void onDisplay() override
    {
        const float sliderW = 50.0f;
        const float sliderH = 200.0f;
        const float buttonH = 30.0f;
        const float spacing = 10.0f;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

        if (ImGui::Begin("RegrooveFX", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("REGROOVE FX");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10.0f));

            // Horizontal layout for all effect sections
            ImGui::BeginChild("EffectsPanel", ImVec2(0, 0), false);

            // DISTORTION
            renderEffectSection("DISTORTION",
                kParameterDistortionEnabled, kParameterDistortionDrive, kParameterDistortionMix,
                "Drive", "Mix", sliderW, sliderH, buttonH, spacing);

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(spacing, 0));
            ImGui::SameLine();

            // FILTER
            renderEffectSection("FILTER",
                kParameterFilterEnabled, kParameterFilterCutoff, kParameterFilterResonance,
                "Cutoff", "Reso", sliderW, sliderH, buttonH, spacing);

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(spacing, 0));
            ImGui::SameLine();

            // EQ (3-band)
            renderEQSection(sliderW, sliderH, buttonH, spacing);

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(spacing, 0));
            ImGui::SameLine();

            // COMPRESSOR
            renderCompressorSection(sliderW, sliderH, buttonH, spacing);

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(spacing, 0));
            ImGui::SameLine();

            // DELAY
            renderEffectSection("DELAY",
                kParameterDelayEnabled, kParameterDelayTime, kParameterDelayFeedback,
                "Time", "FB", sliderW, sliderH, buttonH, spacing, kParameterDelayMix, "Mix");

            ImGui::EndChild();
        }
        ImGui::End();
    }

private:
    float fParameters[kParameterCount];

    void renderEffectSection(const char* name,
                            uint32_t enableParam, uint32_t param1, uint32_t param2,
                            const char* label1, const char* label2,
                            float sliderW, float sliderH, float buttonH, float spacing,
                            uint32_t param3 = 0, const char* label3 = nullptr)
    {
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", name);
        ImGui::Dummy(ImVec2(0, 4.0f));

        // Enable button
        bool enabled = fParameters[enableParam] >= 0.5f;
        ImVec4 btnCol = enabled ? ImVec4(0.70f, 0.60f, 0.20f, 1.0f) : ImVec4(0.26f, 0.27f, 0.30f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, btnCol);

        char btnLabel[64];
        snprintf(btnLabel, sizeof(btnLabel), "ON##%s", name);
        if (ImGui::Button(btnLabel, ImVec2(sliderW, buttonH)))
        {
            enabled = !enabled;
            setParameterValue(enableParam, enabled ? 1.0f : 0.0f);
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, spacing));

        // First parameter slider
        float value1 = fParameters[param1];
        char slider1Label[64];
        snprintf(slider1Label, sizeof(slider1Label), "##%s_1", name);
        if (ImGui::VSliderFloat(slider1Label, ImVec2(sliderW, sliderH), &value1, 0.0f, 1.0f, ""))
        {
            setParameterValue(param1, value1);
        }
        ImGui::Text("%s", label1);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(spacing, 0));
        ImGui::SameLine();

        // Second parameter - aligned with first
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(sliderW, buttonH));
        ImGui::Dummy(ImVec2(0, spacing));

        float value2 = fParameters[param2];
        char slider2Label[64];
        snprintf(slider2Label, sizeof(slider2Label), "##%s_2", name);
        if (ImGui::VSliderFloat(slider2Label, ImVec2(sliderW, sliderH), &value2, 0.0f, 1.0f, ""))
        {
            setParameterValue(param2, value2);
        }
        ImGui::Text("%s", label2);
        ImGui::EndGroup();

        // Optional third parameter
        if (param3 > 0 && label3 != nullptr)
        {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(spacing, 0));
            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Dummy(ImVec2(sliderW, buttonH));
            ImGui::Dummy(ImVec2(0, spacing));

            float value3 = fParameters[param3];
            char slider3Label[64];
            snprintf(slider3Label, sizeof(slider3Label), "##%s_3", name);
            if (ImGui::VSliderFloat(slider3Label, ImVec2(sliderW, sliderH), &value3, 0.0f, 1.0f, ""))
            {
                setParameterValue(param3, value3);
            }
            ImGui::Text("%s", label3);
            ImGui::EndGroup();
        }

        ImGui::EndGroup();
    }

    void renderEQSection(float sliderW, float sliderH, float buttonH, float spacing)
    {
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "EQ");
        ImGui::Dummy(ImVec2(0, 4.0f));

        // Enable button
        bool enabled = fParameters[kParameterEQEnabled] >= 0.5f;
        ImVec4 btnCol = enabled ? ImVec4(0.70f, 0.60f, 0.20f, 1.0f) : ImVec4(0.26f, 0.27f, 0.30f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
        if (ImGui::Button("ON##EQ", ImVec2(sliderW, buttonH)))
        {
            enabled = !enabled;
            setParameterValue(kParameterEQEnabled, enabled ? 1.0f : 0.0f);
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, spacing));

        // Low
        float low = fParameters[kParameterEQLow];
        if (ImGui::VSliderFloat("##eq_low", ImVec2(sliderW, sliderH), &low, 0.0f, 1.0f, ""))
        {
            setParameterValue(kParameterEQLow, low);
        }
        ImGui::Text("Low");

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(spacing, 0));
        ImGui::SameLine();

        // Mid
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(sliderW, buttonH));
        ImGui::Dummy(ImVec2(0, spacing));
        float mid = fParameters[kParameterEQMid];
        if (ImGui::VSliderFloat("##eq_mid", ImVec2(sliderW, sliderH), &mid, 0.0f, 1.0f, ""))
        {
            setParameterValue(kParameterEQMid, mid);
        }
        ImGui::Text("Mid");
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(spacing, 0));
        ImGui::SameLine();

        // High
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(sliderW, buttonH));
        ImGui::Dummy(ImVec2(0, spacing));
        float high = fParameters[kParameterEQHigh];
        if (ImGui::VSliderFloat("##eq_high", ImVec2(sliderW, sliderH), &high, 0.0f, 1.0f, ""))
        {
            setParameterValue(kParameterEQHigh, high);
        }
        ImGui::Text("High");
        ImGui::EndGroup();

        ImGui::EndGroup();
    }

    void renderCompressorSection(float sliderW, float sliderH, float buttonH, float spacing)
    {
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "COMPRESSOR");
        ImGui::Dummy(ImVec2(0, 4.0f));

        // Enable button
        bool enabled = fParameters[kParameterCompressorEnabled] >= 0.5f;
        ImVec4 btnCol = enabled ? ImVec4(0.70f, 0.60f, 0.20f, 1.0f) : ImVec4(0.26f, 0.27f, 0.30f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
        if (ImGui::Button("ON##COMP", ImVec2(sliderW, buttonH)))
        {
            enabled = !enabled;
            setParameterValue(kParameterCompressorEnabled, enabled ? 1.0f : 0.0f);
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, spacing));

        // Threshold
        float thresh = fParameters[kParameterCompressorThreshold];
        if (ImGui::VSliderFloat("##comp_thresh", ImVec2(sliderW, sliderH), &thresh, 0.0f, 1.0f, ""))
        {
            setParameterValue(kParameterCompressorThreshold, thresh);
        }
        ImGui::Text("Thresh");

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(spacing, 0));
        ImGui::SameLine();

        // Ratio
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(sliderW, buttonH));
        ImGui::Dummy(ImVec2(0, spacing));
        float ratio = fParameters[kParameterCompressorRatio];
        if (ImGui::VSliderFloat("##comp_ratio", ImVec2(sliderW, sliderH), &ratio, 0.0f, 1.0f, ""))
        {
            setParameterValue(kParameterCompressorRatio, ratio);
        }
        ImGui::Text("Ratio");
        ImGui::EndGroup();

        ImGui::EndGroup();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveFXUI)
};

// -----------------------------------------------------------------------

UI* createUI()
{
    return new RegrooveFXUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
