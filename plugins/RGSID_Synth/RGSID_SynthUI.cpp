#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"
#include "../../synth/synth_sid.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGSID_SynthUI : public UI
{
public:
    RGSID_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults (matching DSP defaults)
        // Voice 1
        fParameters[kParameterVoice1Waveform] = SID_WAVE_PULSE;
        fParameters[kParameterVoice1PulseWidth] = 0.5f;
        fParameters[kParameterVoice1Attack] = 0.0f;
        fParameters[kParameterVoice1Decay] = 0.5f;
        fParameters[kParameterVoice1Sustain] = 0.7f;
        fParameters[kParameterVoice1Release] = 0.3f;
        fParameters[kParameterVoice1RingMod] = 0.0f;
        fParameters[kParameterVoice1Sync] = 0.0f;

        // Voice 2
        fParameters[kParameterVoice2Waveform] = SID_WAVE_PULSE;
        fParameters[kParameterVoice2PulseWidth] = 0.5f;
        fParameters[kParameterVoice2Attack] = 0.0f;
        fParameters[kParameterVoice2Decay] = 0.5f;
        fParameters[kParameterVoice2Sustain] = 0.7f;
        fParameters[kParameterVoice2Release] = 0.3f;
        fParameters[kParameterVoice2RingMod] = 0.0f;
        fParameters[kParameterVoice2Sync] = 0.0f;

        // Voice 3
        fParameters[kParameterVoice3Waveform] = SID_WAVE_PULSE;
        fParameters[kParameterVoice3PulseWidth] = 0.5f;
        fParameters[kParameterVoice3Attack] = 0.0f;
        fParameters[kParameterVoice3Decay] = 0.5f;
        fParameters[kParameterVoice3Sustain] = 0.7f;
        fParameters[kParameterVoice3Release] = 0.3f;
        fParameters[kParameterVoice3RingMod] = 0.0f;
        fParameters[kParameterVoice3Sync] = 0.0f;

        // Filter
        fParameters[kParameterFilterMode] = SID_FILTER_LP;
        fParameters[kParameterFilterCutoff] = 0.5f;
        fParameters[kParameterFilterResonance] = 0.0f;
        fParameters[kParameterFilterVoice1] = 0.0f;
        fParameters[kParameterFilterVoice2] = 0.0f;
        fParameters[kParameterFilterVoice3] = 0.0f;

        // Global
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RGSIDImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGSID_SynthUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;
            fImGuiWidget->repaint();
        }
    }

    void uiIdle() override
    {
        fImGuiWidget->repaint();
    }

    void uiReshape(uint width, uint height) override
    {
        UI::uiReshape(width, height);
        fImGuiWidget->setSize(width, height);
    }

private:
    friend class RGSIDImGuiWidget;

    float fParameters[kParameterCount];

    class RGSIDImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGSIDImGuiWidget(RGSID_SynthUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RGSID_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.18f, 1.0f));

                ImGui::Spacing();

                // Title
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGSID_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), RGSID_DISPLAY_NAME);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Render all 3 voices
                renderVoice(1, kParameterVoice1Waveform);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                renderVoice(2, kParameterVoice2Waveform);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                renderVoice(3, kParameterVoice3Waveform);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Filter Section
                renderFilter();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Master Section
                ImGui::Text("MASTER");
                ImGui::Spacing();

                float volume = fUI->fParameters[kParameterVolume];
                if (ImGuiKnobs::Knob("Volume", &volume, 0.0f, 1.0f, 0.001f,
                                    "%.0f%%", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVolume] = volume;
                    fUI->setParameterValue(kParameterVolume, volume);
                }

                ImGui::Spacing();
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        void renderVoice(int voiceNum, uint32_t baseParam)
        {
            char title[32];
            snprintf(title, sizeof(title), "VOICE %d", voiceNum);
            ImGui::Text("%s", title);
            ImGui::Spacing();

            // Waveform selection (checkboxes - SID allows combining waveforms)
            uint8_t waveform = (uint8_t)(fUI->fParameters[baseParam] + 0.5f);
            bool triangle = waveform & SID_WAVE_TRIANGLE;
            bool sawtooth = waveform & SID_WAVE_SAWTOOTH;
            bool pulse = waveform & SID_WAVE_PULSE;
            bool noise = waveform & SID_WAVE_NOISE;

            char labelTriangle[32], labelSawtooth[32], labelPulse[32], labelNoise[32];
            snprintf(labelTriangle, sizeof(labelTriangle), "Triangle##v%d", voiceNum);
            snprintf(labelSawtooth, sizeof(labelSawtooth), "Sawtooth##v%d", voiceNum);
            snprintf(labelPulse, sizeof(labelPulse), "Pulse##v%d", voiceNum);
            snprintf(labelNoise, sizeof(labelNoise), "Noise##v%d", voiceNum);

            if (ImGui::Checkbox(labelTriangle, &triangle)) {
                waveform = triangle ? (waveform | SID_WAVE_TRIANGLE) : (waveform & ~SID_WAVE_TRIANGLE);
                fUI->fParameters[baseParam] = (float)waveform;
                fUI->setParameterValue(baseParam, (float)waveform);
            }

            ImGui::SameLine();

            if (ImGui::Checkbox(labelSawtooth, &sawtooth)) {
                waveform = sawtooth ? (waveform | SID_WAVE_SAWTOOTH) : (waveform & ~SID_WAVE_SAWTOOTH);
                fUI->fParameters[baseParam] = (float)waveform;
                fUI->setParameterValue(baseParam, (float)waveform);
            }

            if (ImGui::Checkbox(labelPulse, &pulse)) {
                waveform = pulse ? (waveform | SID_WAVE_PULSE) : (waveform & ~SID_WAVE_PULSE);
                fUI->fParameters[baseParam] = (float)waveform;
                fUI->setParameterValue(baseParam, (float)waveform);
            }

            ImGui::SameLine();

            if (ImGui::Checkbox(labelNoise, &noise)) {
                waveform = noise ? (waveform | SID_WAVE_NOISE) : (waveform & ~SID_WAVE_NOISE);
                fUI->fParameters[baseParam] = (float)waveform;
                fUI->setParameterValue(baseParam, (float)waveform);
            }

            ImGui::Spacing();

            // Pulse Width knob - unique ID per voice
            char labelPW[32], labelA[32], labelD[32], labelS[32], labelR[32];
            snprintf(labelPW, sizeof(labelPW), "PW##pw%d", voiceNum);
            snprintf(labelA, sizeof(labelA), "A##a%d", voiceNum);
            snprintf(labelD, sizeof(labelD), "D##d%d", voiceNum);
            snprintf(labelS, sizeof(labelS), "S##s%d", voiceNum);
            snprintf(labelR, sizeof(labelR), "R##r%d", voiceNum);

            float pulseWidth = fUI->fParameters[baseParam + 1];
            if (ImGuiKnobs::Knob(labelPW, &pulseWidth, 0.0f, 1.0f, 0.001f,
                                "%.0f%%", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[baseParam + 1] = pulseWidth;
                fUI->setParameterValue(baseParam + 1, pulseWidth);
            }

            ImGui::SameLine();

            // ADSR knobs
            float attack = fUI->fParameters[baseParam + 2];
            if (ImGuiKnobs::Knob(labelA, &attack, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[baseParam + 2] = attack;
                fUI->setParameterValue(baseParam + 2, attack);
            }

            ImGui::SameLine();

            float decay = fUI->fParameters[baseParam + 3];
            if (ImGuiKnobs::Knob(labelD, &decay, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[baseParam + 3] = decay;
                fUI->setParameterValue(baseParam + 3, decay);
            }

            ImGui::SameLine();

            float sustain = fUI->fParameters[baseParam + 4];
            if (ImGuiKnobs::Knob(labelS, &sustain, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[baseParam + 4] = sustain;
                fUI->setParameterValue(baseParam + 4, sustain);
            }

            ImGui::SameLine();

            float release = fUI->fParameters[baseParam + 5];
            if (ImGuiKnobs::Knob(labelR, &release, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[baseParam + 5] = release;
                fUI->setParameterValue(baseParam + 5, release);
            }

            ImGui::Spacing();

            // Ring Mod and Sync checkboxes
            char labelRingMod[32], labelSync[32];
            snprintf(labelRingMod, sizeof(labelRingMod), "Ring Mod##rm%d", voiceNum);
            snprintf(labelSync, sizeof(labelSync), "Sync##sy%d", voiceNum);

            bool ringMod = fUI->fParameters[baseParam + 6] > 0.5f;
            if (ImGui::Checkbox(labelRingMod, &ringMod)) {
                fUI->fParameters[baseParam + 6] = ringMod ? 1.0f : 0.0f;
                fUI->setParameterValue(baseParam + 6, ringMod ? 1.0f : 0.0f);
            }

            ImGui::SameLine();

            bool sync = fUI->fParameters[baseParam + 7] > 0.5f;
            if (ImGui::Checkbox(labelSync, &sync)) {
                fUI->fParameters[baseParam + 7] = sync ? 1.0f : 0.0f;
                fUI->setParameterValue(baseParam + 7, sync ? 1.0f : 0.0f);
            }
        }

        void renderFilter()
        {
            ImGui::Text("FILTER");
            ImGui::Spacing();

            // Filter mode selection (radio buttons)
            int filterMode = (int)(fUI->fParameters[kParameterFilterMode] + 0.5f);

            if (ImGui::RadioButton("LP", &filterMode, SID_FILTER_LP)) {
                fUI->fParameters[kParameterFilterMode] = (float)filterMode;
                fUI->setParameterValue(kParameterFilterMode, (float)filterMode);
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("BP", &filterMode, SID_FILTER_BP)) {
                fUI->fParameters[kParameterFilterMode] = (float)filterMode;
                fUI->setParameterValue(kParameterFilterMode, (float)filterMode);
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("HP", &filterMode, SID_FILTER_HP)) {
                fUI->fParameters[kParameterFilterMode] = (float)filterMode;
                fUI->setParameterValue(kParameterFilterMode, (float)filterMode);
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("OFF", &filterMode, SID_FILTER_OFF)) {
                fUI->fParameters[kParameterFilterMode] = (float)filterMode;
                fUI->setParameterValue(kParameterFilterMode, (float)filterMode);
            }

            ImGui::Spacing();

            // Filter knobs
            float cutoff = fUI->fParameters[kParameterFilterCutoff];
            if (ImGuiKnobs::Knob("Cutoff", &cutoff, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[kParameterFilterCutoff] = cutoff;
                fUI->setParameterValue(kParameterFilterCutoff, cutoff);
            }

            ImGui::SameLine();

            float resonance = fUI->fParameters[kParameterFilterResonance];
            if (ImGuiKnobs::Knob("Resonance", &resonance, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[kParameterFilterResonance] = resonance;
                fUI->setParameterValue(kParameterFilterResonance, resonance);
            }

            ImGui::Spacing();

            // Voice routing checkboxes
            bool v1 = fUI->fParameters[kParameterFilterVoice1] > 0.5f;
            if (ImGui::Checkbox("V1", &v1)) {
                fUI->fParameters[kParameterFilterVoice1] = v1 ? 1.0f : 0.0f;
                fUI->setParameterValue(kParameterFilterVoice1, v1 ? 1.0f : 0.0f);
            }

            ImGui::SameLine();

            bool v2 = fUI->fParameters[kParameterFilterVoice2] > 0.5f;
            if (ImGui::Checkbox("V2", &v2)) {
                fUI->fParameters[kParameterFilterVoice2] = v2 ? 1.0f : 0.0f;
                fUI->setParameterValue(kParameterFilterVoice2, v2 ? 1.0f : 0.0f);
            }

            ImGui::SameLine();

            bool v3 = fUI->fParameters[kParameterFilterVoice3] > 0.5f;
            if (ImGui::Checkbox("V3", &v3)) {
                fUI->fParameters[kParameterFilterVoice3] = v3 ? 1.0f : 0.0f;
                fUI->setParameterValue(kParameterFilterVoice3, v3 ? 1.0f : 0.0f);
            }
        }

        RGSID_SynthUI* const fUI;
    };

    RGSIDImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSID_SynthUI)
};

UI* createUI()
{
    return new RGSID_SynthUI();
}

END_NAMESPACE_DISTRHO
