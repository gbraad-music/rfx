#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG20_SynthUI : public UI
{
public:
    RG20_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults (matching DSP defaults)
        fParameters[kParameterVCO1Waveform] = 0.0f;
        fParameters[kParameterVCO1Octave] = 0.5f;
        fParameters[kParameterVCO1Tune] = 0.5f;
        fParameters[kParameterVCO1Level] = 0.7f;

        fParameters[kParameterVCO2Waveform] = 0.0f;
        fParameters[kParameterVCO2Octave] = 0.5f;
        fParameters[kParameterVCO2Tune] = 0.5f;
        fParameters[kParameterVCO2Level] = 0.5f;

        fParameters[kParameterNoiseLevel] = 0.0f;
        fParameters[kParameterRingModLevel] = 0.0f;

        fParameters[kParameterHPFCutoff] = 0.1f;
        fParameters[kParameterHPFPeak] = 0.0f;

        fParameters[kParameterLPFCutoff] = 0.8f;
        fParameters[kParameterLPFPeak] = 0.3f;

        fParameters[kParameterFilterAttack] = 0.01f;
        fParameters[kParameterFilterDecay] = 0.3f;
        fParameters[kParameterFilterRelease] = 0.1f;
        fParameters[kParameterFilterEnvAmount] = 0.5f;

        fParameters[kParameterAmpAttack] = 0.01f;
        fParameters[kParameterAmpDecay] = 0.3f;
        fParameters[kParameterAmpRelease] = 0.1f;

        fParameters[kParameterLFORate] = 5.0f;
        fParameters[kParameterLFOPitchDepth] = 0.0f;
        fParameters[kParameterLFOFilterDepth] = 0.0f;

        fParameters[kParameterPortamento] = 0.0f;
        fParameters[kParameterVolume] = 0.5f;

        fImGuiWidget = new RG20ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG20_SynthUI() override
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
    friend class RG20ImGuiWidget;

    float fParameters[kParameterCount];

    class RG20ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG20ImGuiWidget(RG20_SynthUI* const ui)
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

            if (ImGui::Begin(RG20_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG20_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "%s", RG20_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                const float knobSize = 55.0f;
                const float sectionWidth = (width - 60.0f) / 5.0f;

                ImGui::Columns(5, "sections", false);
                for (int i = 0; i < 5; i++) {
                    ImGui::SetColumnWidth(i, sectionWidth);
                }

                // === VCO 1 SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("VCO 1");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // Waveform
                float vco1Wave = fUI->fParameters[kParameterVCO1Waveform];
                if (ImGuiKnobs::Knob("Wave", &vco1Wave, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO1Waveform] = vco1Wave;
                    fUI->setParameterValue(kParameterVCO1Waveform, vco1Wave);
                }

                // Octave
                float vco1Oct = fUI->fParameters[kParameterVCO1Octave];
                if (ImGuiKnobs::Knob("Octave", &vco1Oct, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO1Octave] = vco1Oct;
                    fUI->setParameterValue(kParameterVCO1Octave, vco1Oct);
                }

                // Tune
                float vco1Tune = fUI->fParameters[kParameterVCO1Tune];
                if (ImGuiKnobs::Knob("Tune", &vco1Tune, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO1Tune] = vco1Tune;
                    fUI->setParameterValue(kParameterVCO1Tune, vco1Tune);
                }

                // Level
                float vco1Level = fUI->fParameters[kParameterVCO1Level];
                if (ImGuiKnobs::Knob("Level", &vco1Level, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO1Level] = vco1Level;
                    fUI->setParameterValue(kParameterVCO1Level, vco1Level);
                }

                ImGui::NextColumn();

                // === VCO 2 SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("VCO 2");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                float vco2Wave = fUI->fParameters[kParameterVCO2Waveform];
                if (ImGuiKnobs::Knob("Wave", &vco2Wave, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO2Waveform] = vco2Wave;
                    fUI->setParameterValue(kParameterVCO2Waveform, vco2Wave);
                }

                float vco2Oct = fUI->fParameters[kParameterVCO2Octave];
                if (ImGuiKnobs::Knob("Octave", &vco2Oct, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO2Octave] = vco2Oct;
                    fUI->setParameterValue(kParameterVCO2Octave, vco2Oct);
                }

                float vco2Tune = fUI->fParameters[kParameterVCO2Tune];
                if (ImGuiKnobs::Knob("Tune", &vco2Tune, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO2Tune] = vco2Tune;
                    fUI->setParameterValue(kParameterVCO2Tune, vco2Tune);
                }

                float vco2Level = fUI->fParameters[kParameterVCO2Level];
                if (ImGuiKnobs::Knob("Level", &vco2Level, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVCO2Level] = vco2Level;
                    fUI->setParameterValue(kParameterVCO2Level, vco2Level);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Mixer section (Noise & Ring Mod)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::Text("MIXER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                float noiseLevel = fUI->fParameters[kParameterNoiseLevel];
                if (ImGuiKnobs::Knob("Noise", &noiseLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterNoiseLevel] = noiseLevel;
                    fUI->setParameterValue(kParameterNoiseLevel, noiseLevel);
                }

                float ringMod = fUI->fParameters[kParameterRingModLevel];
                if (ImGuiKnobs::Knob("Ring Mod", &ringMod, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterRingModLevel] = ringMod;
                    fUI->setParameterValue(kParameterRingModLevel, ringMod);
                }

                ImGui::NextColumn();

                // === FILTER SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("FILTERS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                ImGui::Text("Highpass");
                float hpfCutoff = fUI->fParameters[kParameterHPFCutoff];
                if (ImGuiKnobs::Knob("HPF Cut", &hpfCutoff, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterHPFCutoff] = hpfCutoff;
                    fUI->setParameterValue(kParameterHPFCutoff, hpfCutoff);
                }

                float hpfPeak = fUI->fParameters[kParameterHPFPeak];
                if (ImGuiKnobs::Knob("HPF Peak", &hpfPeak, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterHPFPeak] = hpfPeak;
                    fUI->setParameterValue(kParameterHPFPeak, hpfPeak);
                }

                ImGui::Spacing();
                ImGui::Text("Lowpass");
                float lpfCutoff = fUI->fParameters[kParameterLPFCutoff];
                if (ImGuiKnobs::Knob("LPF Cut", &lpfCutoff, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLPFCutoff] = lpfCutoff;
                    fUI->setParameterValue(kParameterLPFCutoff, lpfCutoff);
                }

                float lpfPeak = fUI->fParameters[kParameterLPFPeak];
                if (ImGuiKnobs::Knob("LPF Peak", &lpfPeak, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLPFPeak] = lpfPeak;
                    fUI->setParameterValue(kParameterLPFPeak, lpfPeak);
                }

                ImGui::NextColumn();

                // === ENVELOPE SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("ENVELOPES");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                ImGui::Text("Filter Env (ADR)");
                float filtAttack = fUI->fParameters[kParameterFilterAttack];
                if (ImGuiKnobs::Knob("A##filt", &filtAttack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterAttack] = filtAttack;
                    fUI->setParameterValue(kParameterFilterAttack, filtAttack);
                }
                ImGui::SameLine();
                float filtDecay = fUI->fParameters[kParameterFilterDecay];
                if (ImGuiKnobs::Knob("D##filt", &filtDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterDecay] = filtDecay;
                    fUI->setParameterValue(kParameterFilterDecay, filtDecay);
                }
                ImGui::SameLine();
                float filtRelease = fUI->fParameters[kParameterFilterRelease];
                if (ImGuiKnobs::Knob("R##filt", &filtRelease, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterRelease] = filtRelease;
                    fUI->setParameterValue(kParameterFilterRelease, filtRelease);
                }

                float filtEnvAmt = fUI->fParameters[kParameterFilterEnvAmount];
                if (ImGuiKnobs::Knob("Env Amt", &filtEnvAmt, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterEnvAmount] = filtEnvAmt;
                    fUI->setParameterValue(kParameterFilterEnvAmount, filtEnvAmt);
                }

                ImGui::Spacing();
                ImGui::Text("Amp Env (ADR)");
                float ampAttack = fUI->fParameters[kParameterAmpAttack];
                if (ImGuiKnobs::Knob("A##amp", &ampAttack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpAttack] = ampAttack;
                    fUI->setParameterValue(kParameterAmpAttack, ampAttack);
                }
                ImGui::SameLine();
                float ampDecay = fUI->fParameters[kParameterAmpDecay];
                if (ImGuiKnobs::Knob("D##amp", &ampDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpDecay] = ampDecay;
                    fUI->setParameterValue(kParameterAmpDecay, ampDecay);
                }
                ImGui::SameLine();
                float ampRelease = fUI->fParameters[kParameterAmpRelease];
                if (ImGuiKnobs::Knob("R##amp", &ampRelease, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 48, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpRelease] = ampRelease;
                    fUI->setParameterValue(kParameterAmpRelease, ampRelease);
                }

                ImGui::NextColumn();

                // === MODULATION & PERFORMANCE ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("MODULATION");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                float lfoRate = fUI->fParameters[kParameterLFORate];
                if (ImGuiKnobs::Knob("LFO Rate", &lfoRate, 0.1f, 20.0f, 0.01f,
                                    "%.1f Hz", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFORate] = lfoRate;
                    fUI->setParameterValue(kParameterLFORate, lfoRate);
                }

                float lfoPitch = fUI->fParameters[kParameterLFOPitchDepth];
                if (ImGuiKnobs::Knob("LFO Pitch", &lfoPitch, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOPitchDepth] = lfoPitch;
                    fUI->setParameterValue(kParameterLFOPitchDepth, lfoPitch);
                }

                float lfoFilter = fUI->fParameters[kParameterLFOFilterDepth];
                if (ImGuiKnobs::Knob("LFO Filter", &lfoFilter, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOFilterDepth] = lfoFilter;
                    fUI->setParameterValue(kParameterLFOFilterDepth, lfoFilter);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                float portamento = fUI->fParameters[kParameterPortamento];
                if (ImGuiKnobs::Knob("Portamento", &portamento, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterPortamento] = portamento;
                    fUI->setParameterValue(kParameterPortamento, portamento);
                }

                float volume = fUI->fParameters[kParameterVolume];
                if (ImGuiKnobs::Knob("Volume", &volume, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVolume] = volume;
                    fUI->setParameterValue(kParameterVolume, volume);
                }

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RG20_SynthUI* const fUI;
    };

    RG20ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG20_SynthUI)
};

UI* createUI()
{
    return new RG20_SynthUI();
}

END_NAMESPACE_DISTRHO
