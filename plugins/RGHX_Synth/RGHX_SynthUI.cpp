/*
 * RGHX Synth UI
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

enum Parameters {
    kParameterWaveform = 0,
    kParameterWaveLength,
    kParameterAttack,
    kParameterDecay,
    kParameterSustain,
    kParameterRelease,
    kParameterFilterType,
    kParameterFilterCutoff,
    kParameterFilterResonance,
    kParameterVibratoDepth,
    kParameterVibratoSpeed,
    kParameterVolume,
    kParameterCount
};

class RGHX_SynthUI : public UI
{
public:
    RGHX_SynthUI()
        : UI(700, 500)
    {
        setGeometryConstraints(700, 500, true);

        // Initialize parameters with defaults
        fParameters[kParameterWaveform] = 1.0f;       // Sawtooth
        fParameters[kParameterWaveLength] = 0.125f;   // 32 samples
        fParameters[kParameterAttack] = 0.01f;
        fParameters[kParameterDecay] = 0.1f;
        fParameters[kParameterSustain] = 0.7f;
        fParameters[kParameterRelease] = 0.1f;
        fParameters[kParameterFilterType] = 1.0f;     // Lowpass
        fParameters[kParameterFilterCutoff] = 1.0f;
        fParameters[kParameterFilterResonance] = 0.0f;
        fParameters[kParameterVibratoDepth] = 0.0f;
        fParameters[kParameterVibratoSpeed] = 0.0f;
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RGHXImGuiWidget(this);
        fImGuiWidget->setSize(700, 500);
    }

    ~RGHX_SynthUI() override
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
    friend class RGHXImGuiWidget;

    float fParameters[kParameterCount];

    class RGHXImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGHXImGuiWidget(RGHX_SynthUI* const ui)
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

            if (ImGui::Begin(RGHX_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                ImGui::Spacing();

                // Title with accent color
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGHX_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), RGHX_DISPLAY_NAME);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Oscillator Section
                ImGui::Text("OSCILLATOR");
                ImGui::Spacing();

                const char* waveforms[] = { "Triangle", "Sawtooth", "Square", "Noise" };
                int waveformIdx = (int)(fUI->fParameters[kParameterWaveform] + 0.5f);
                if (ImGui::Combo("Waveform", &waveformIdx, waveforms, 4)) {
                    fUI->fParameters[kParameterWaveform] = (float)waveformIdx;
                    fUI->setParameterValue(kParameterWaveform, (float)waveformIdx);
                }

                ImGui::Spacing();

                float wavelength = fUI->fParameters[kParameterWaveLength];
                if (ImGuiKnobs::Knob("Wave Length", &wavelength, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterWaveLength] = wavelength;
                    fUI->setParameterValue(kParameterWaveLength, wavelength);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Envelope Section
                ImGui::Text("ENVELOPE");
                ImGui::Spacing();

                float attack = fUI->fParameters[kParameterAttack];
                if (ImGuiKnobs::Knob("Attack", &attack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAttack] = attack;
                    fUI->setParameterValue(kParameterAttack, attack);
                }

                ImGui::SameLine();

                float decay = fUI->fParameters[kParameterDecay];
                if (ImGuiKnobs::Knob("Decay", &decay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterDecay] = decay;
                    fUI->setParameterValue(kParameterDecay, decay);
                }

                ImGui::SameLine();

                float sustain = fUI->fParameters[kParameterSustain];
                if (ImGuiKnobs::Knob("Sustain", &sustain, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSustain] = sustain;
                    fUI->setParameterValue(kParameterSustain, sustain);
                }

                ImGui::SameLine();

                float release = fUI->fParameters[kParameterRelease];
                if (ImGuiKnobs::Knob("Release", &release, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterRelease] = release;
                    fUI->setParameterValue(kParameterRelease, release);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Filter Section
                ImGui::Text("FILTER");
                ImGui::Spacing();

                const char* filtertypes[] = { "None", "Lowpass", "Highpass" };
                int filterTypeIdx = (int)(fUI->fParameters[kParameterFilterType] + 0.5f);
                if (ImGui::Combo("Filter Type", &filterTypeIdx, filtertypes, 3)) {
                    fUI->fParameters[kParameterFilterType] = (float)filterTypeIdx;
                    fUI->setParameterValue(kParameterFilterType, (float)filterTypeIdx);
                }

                ImGui::Spacing();

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
                ImGui::Separator();
                ImGui::Spacing();

                // Modulation Section
                ImGui::Text("MODULATION");
                ImGui::Spacing();

                float vibdepth = fUI->fParameters[kParameterVibratoDepth];
                if (ImGuiKnobs::Knob("Vib Depth", &vibdepth, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVibratoDepth] = vibdepth;
                    fUI->setParameterValue(kParameterVibratoDepth, vibdepth);
                }

                ImGui::SameLine();

                float vibspeed = fUI->fParameters[kParameterVibratoSpeed];
                if (ImGuiKnobs::Knob("Vib Speed", &vibspeed, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVibratoSpeed] = vibspeed;
                    fUI->setParameterValue(kParameterVibratoSpeed, vibspeed);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Output Section
                ImGui::Text("OUTPUT");
                ImGui::Spacing();

                float volume = fUI->fParameters[kParameterVolume];
                if (ImGuiKnobs::Knob("Volume", &volume, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVolume] = volume;
                    fUI->setParameterValue(kParameterVolume, volume);
                }

                ImGui::Spacing();
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RGHX_SynthUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGHXImGuiWidget)
    };

    RGHXImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGHX_SynthUI)
};

UI* createUI()
{
    return new RGHX_SynthUI();
}

END_NAMESPACE_DISTRHO
