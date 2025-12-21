#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGK1_SynthUI : public UI
{
public:
    RGK1_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults
        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Set specific defaults (matching plugin)
        fParameters[kParameterDCO1Wave] = 0.0f;
        fParameters[kParameterDCO1Octave] = 0.0f;
        fParameters[kParameterDCO1Level] = 0.7f;
        fParameters[kParameterDCO1Detune] = 0.0f;

        fParameters[kParameterDCO2Wave] = 0.125f;
        fParameters[kParameterDCO2Octave] = 0.0f;
        fParameters[kParameterDCO2Level] = 0.7f;
        fParameters[kParameterDCO2Detune] = 0.0f;

        fParameters[kParameterDCF1Cutoff] = 0.7f;
        fParameters[kParameterDCF1Resonance] = 0.3f;
        fParameters[kParameterDCF1EnvDepth] = 0.5f;
        fParameters[kParameterDCF1KeyTrack] = 0.3f;

        fParameters[kParameterDCF2Cutoff] = 0.7f;
        fParameters[kParameterDCF2Resonance] = 0.3f;
        fParameters[kParameterDCF2EnvDepth] = 0.5f;
        fParameters[kParameterDCF2KeyTrack] = 0.3f;

        fParameters[kParameterFiltAttack] = 0.01f;
        fParameters[kParameterFiltDecay] = 0.3f;
        fParameters[kParameterFiltSustain] = 0.5f;
        fParameters[kParameterFiltRelease] = 0.5f;

        fParameters[kParameterAmpAttack] = 0.01f;
        fParameters[kParameterAmpDecay] = 0.3f;
        fParameters[kParameterAmpSustain] = 0.7f;
        fParameters[kParameterAmpRelease] = 0.5f;

        fParameters[kParameterLFOWave] = 0.0f;
        fParameters[kParameterLFORate] = 5.0f;
        fParameters[kParameterLFOPitchDepth] = 0.0f;
        fParameters[kParameterLFOFilterDepth] = 0.0f;
        fParameters[kParameterLFOAmpDepth] = 0.0f;

        fParameters[kParameterVelocitySensitivity] = 0.5f;
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RGK1ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGK1_SynthUI() override
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
    friend class RGK1ImGuiWidget;

    float fParameters[kParameterCount];

    class RGK1ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGK1ImGuiWidget(RGK1_SynthUI* const ui)
            : ImGuiSubWidget(ui), fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RGK1_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGK1_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", RGK1_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                const float knobSize = 50.0f;

                // === ROW 1: DCO1 + DCF1 ===
                ImGui::Columns(2, "row1", false);

                // DCO 1
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.3f, 1.0f));
                ImGui::Text("DCO 1");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterDCO1Wave, "Wave", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterDCO1Octave, "Octave", -2.0f, 2.0f, "%.0f", knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCO1Level, "Level", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterDCO1Detune, "Detune", -1.0f, 1.0f, "%.2f", knobSize);

                ImGui::NextColumn();

                // DCF 1
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));
                ImGui::Text("DCF 1");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterDCF1Cutoff, "Cutoff", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF1Resonance, "Res", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF1EnvDepth, "Env", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF1KeyTrack, "KeyTrk", 0.0f, 1.0f, knobSize);

                ImGui::Columns(1);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === ROW 2: DCO2 + DCF2 ===
                ImGui::Columns(2, "row2", false);

                // DCO 2
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                ImGui::Text("DCO 2");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterDCO2Wave, "Wave", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterDCO2Octave, "Octave", -2.0f, 2.0f, "%.0f", knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCO2Level, "Level", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterDCO2Detune, "Detune", -1.0f, 1.0f, "%.2f", knobSize);

                ImGui::NextColumn();

                // DCF 2
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.8f, 1.0f));
                ImGui::Text("DCF 2");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterDCF2Cutoff, "Cutoff", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF2Resonance, "Res", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF2EnvDepth, "Env", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDCF2KeyTrack, "KeyTrk", 0.0f, 1.0f, knobSize);

                ImGui::Columns(1);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === ROW 3: Envelopes + LFO + Master ===
                ImGui::Columns(3, "row3", false);

                // Filter Envelope
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.5f, 1.0f, 1.0f));
                ImGui::Text("FILTER ENV");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterFiltAttack, "A##filt", 0.0f, 1.0f, 45.0f);
                ImGui::SameLine();
                KNOB(kParameterFiltDecay, "D##filt", 0.0f, 1.0f, 45.0f);

                KNOB(kParameterFiltSustain, "S##filt", 0.0f, 1.0f, 45.0f);
                ImGui::SameLine();
                KNOB(kParameterFiltRelease, "R##filt", 0.0f, 1.0f, 45.0f);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("AMP ENV");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterAmpAttack, "A##amp", 0.0f, 1.0f, 45.0f);
                ImGui::SameLine();
                KNOB(kParameterAmpDecay, "D##amp", 0.0f, 1.0f, 45.0f);

                KNOB(kParameterAmpSustain, "S##amp", 0.0f, 1.0f, 45.0f);
                ImGui::SameLine();
                KNOB(kParameterAmpRelease, "R##amp", 0.0f, 1.0f, 45.0f);

                ImGui::NextColumn();

                // LFO
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("LFO");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterLFOWave, "Wave", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterLFORate, "Rate", 0.1f, 20.0f, "%.1f Hz", knobSize);

                KNOB(kParameterLFOPitchDepth, "Pitch", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterLFOFilterDepth, "Filter", 0.0f, 1.0f, knobSize);

                KNOB(kParameterLFOAmpDepth, "Amp", 0.0f, 1.0f, knobSize);

                ImGui::NextColumn();

                // Master
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVelocitySensitivity, "Velocity", 0.0f, 1.0f, knobSize);
                KNOB(kParameterVolume, "Volume", 0.0f, 1.0f, knobSize);

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RGK1_SynthUI* const fUI;

        void KNOB(uint32_t param, const char* label, float min, float max, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.001f,
                                "", ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }

        void KNOB_RANGE(uint32_t param, const char* label, float min, float max, const char* format, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.01f,
                                format, ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RGK1ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGK1_SynthUI)
};

UI* createUI()
{
    return new RGK1_SynthUI();
}

END_NAMESPACE_DISTRHO
