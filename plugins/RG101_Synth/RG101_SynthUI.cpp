#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG101_SynthUI : public UI
{
public:
    RG101_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults (matching DSP defaults)
        fParameters[kParameterSawLevel] = 0.8f;
        fParameters[kParameterSquareLevel] = 0.0f;
        fParameters[kParameterSubLevel] = 0.3f;
        fParameters[kParameterNoiseLevel] = 0.0f;
        fParameters[kParameterPulseWidth] = 0.5f;
        fParameters[kParameterPWMDepth] = 0.0f;
        fParameters[kParameterCutoff] = 0.5f;
        fParameters[kParameterResonance] = 0.3f;
        fParameters[kParameterEnvMod] = 0.5f;
        fParameters[kParameterKeyboardTracking] = 0.5f;
        fParameters[kParameterFilterAttack] = 0.01f;
        fParameters[kParameterFilterDecay] = 0.3f;
        fParameters[kParameterFilterSustain] = 0.0f;
        fParameters[kParameterFilterRelease] = 0.1f;
        fParameters[kParameterAmpAttack] = 0.01f;
        fParameters[kParameterAmpDecay] = 0.3f;
        fParameters[kParameterAmpSustain] = 0.7f;
        fParameters[kParameterAmpRelease] = 0.1f;
        fParameters[kParameterLFOWaveform] = 0.0f;
        fParameters[kParameterLFORate] = 0.25f; // 5Hz mapped to 0-1
        fParameters[kParameterLFOPitchDepth] = 0.0f;
        fParameters[kParameterLFOFilterDepth] = 0.0f;
        fParameters[kParameterLFOAmpDepth] = 0.0f;
        fParameters[kParameterVelocitySensitivity] = 0.5f;
        fParameters[kParameterPortamento] = 0.0f;
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RG101ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG101_SynthUI() override
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
    friend class RG101ImGuiWidget;

    float fParameters[kParameterCount];

    class RG101ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG101ImGuiWidget(RG101_SynthUI* const ui)
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

            if (ImGui::Begin(RG101_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // Large font
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG101_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "%s", RG101_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Layout: 4 sections in columns
                const float sectionWidth = (width - 40.0f) / 4.0f;
                const float knobSize = 60.0f;

                ImGui::Columns(4, "sections", false);
                ImGui::SetColumnWidth(0, sectionWidth);
                ImGui::SetColumnWidth(1, sectionWidth);
                ImGui::SetColumnWidth(2, sectionWidth);
                ImGui::SetColumnWidth(3, sectionWidth);

                // === OSCILLATOR SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("OSCILLATOR");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // Saw Level
                float sawLevel = fUI->fParameters[kParameterSawLevel];
                if (ImGuiKnobs::Knob("Saw", &sawLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSawLevel] = sawLevel;
                    fUI->setParameterValue(kParameterSawLevel, sawLevel);
                }

                // Square Level
                float squareLevel = fUI->fParameters[kParameterSquareLevel];
                if (ImGuiKnobs::Knob("Square", &squareLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSquareLevel] = squareLevel;
                    fUI->setParameterValue(kParameterSquareLevel, squareLevel);
                }

                // Sub Level
                float subLevel = fUI->fParameters[kParameterSubLevel];
                if (ImGuiKnobs::Knob("Sub", &subLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSubLevel] = subLevel;
                    fUI->setParameterValue(kParameterSubLevel, subLevel);
                }

                // Noise Level
                float noiseLevel = fUI->fParameters[kParameterNoiseLevel];
                if (ImGuiKnobs::Knob("Noise", &noiseLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterNoiseLevel] = noiseLevel;
                    fUI->setParameterValue(kParameterNoiseLevel, noiseLevel);
                }

                // Pulse Width
                float pulseWidth = fUI->fParameters[kParameterPulseWidth];
                if (ImGuiKnobs::Knob("PW", &pulseWidth, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterPulseWidth] = pulseWidth;
                    fUI->setParameterValue(kParameterPulseWidth, pulseWidth);
                }

                // PWM Depth
                float pwmDepth = fUI->fParameters[kParameterPWMDepth];
                if (ImGuiKnobs::Knob("PWM", &pwmDepth, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterPWMDepth] = pwmDepth;
                    fUI->setParameterValue(kParameterPWMDepth, pwmDepth);
                }

                ImGui::NextColumn();

                // === FILTER SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("FILTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // Cutoff
                float cutoff = fUI->fParameters[kParameterCutoff];
                if (ImGuiKnobs::Knob("Cutoff", &cutoff, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterCutoff] = cutoff;
                    fUI->setParameterValue(kParameterCutoff, cutoff);
                }

                // Resonance
                float resonance = fUI->fParameters[kParameterResonance];
                if (ImGuiKnobs::Knob("Resonance", &resonance, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterResonance] = resonance;
                    fUI->setParameterValue(kParameterResonance, resonance);
                }

                // Env Mod
                float envMod = fUI->fParameters[kParameterEnvMod];
                if (ImGuiKnobs::Knob("Env Mod", &envMod, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterEnvMod] = envMod;
                    fUI->setParameterValue(kParameterEnvMod, envMod);
                }

                // Keyboard Tracking
                float kbdTrack = fUI->fParameters[kParameterKeyboardTracking];
                if (ImGuiKnobs::Knob("Kbd Track", &kbdTrack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterKeyboardTracking] = kbdTrack;
                    fUI->setParameterValue(kParameterKeyboardTracking, kbdTrack);
                }

                ImGui::Spacing();
                ImGui::Text("Filter Env");

                // Filter ADSR
                float filtAttack = fUI->fParameters[kParameterFilterAttack];
                if (ImGuiKnobs::Knob("A##filt", &filtAttack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterAttack] = filtAttack;
                    fUI->setParameterValue(kParameterFilterAttack, filtAttack);
                }
                ImGui::SameLine();
                float filtDecay = fUI->fParameters[kParameterFilterDecay];
                if (ImGuiKnobs::Knob("D##filt", &filtDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterDecay] = filtDecay;
                    fUI->setParameterValue(kParameterFilterDecay, filtDecay);
                }

                float filtSustain = fUI->fParameters[kParameterFilterSustain];
                if (ImGuiKnobs::Knob("S##filt", &filtSustain, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterSustain] = filtSustain;
                    fUI->setParameterValue(kParameterFilterSustain, filtSustain);
                }
                ImGui::SameLine();
                float filtRelease = fUI->fParameters[kParameterFilterRelease];
                if (ImGuiKnobs::Knob("R##filt", &filtRelease, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterFilterRelease] = filtRelease;
                    fUI->setParameterValue(kParameterFilterRelease, filtRelease);
                }

                ImGui::NextColumn();

                // === ENVELOPE SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("AMPLIFIER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                ImGui::Text("Amp Env");

                // Amp ADSR
                float ampAttack = fUI->fParameters[kParameterAmpAttack];
                if (ImGuiKnobs::Knob("A##amp", &ampAttack, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpAttack] = ampAttack;
                    fUI->setParameterValue(kParameterAmpAttack, ampAttack);
                }
                ImGui::SameLine();
                float ampDecay = fUI->fParameters[kParameterAmpDecay];
                if (ImGuiKnobs::Knob("D##amp", &ampDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpDecay] = ampDecay;
                    fUI->setParameterValue(kParameterAmpDecay, ampDecay);
                }

                float ampSustain = fUI->fParameters[kParameterAmpSustain];
                if (ImGuiKnobs::Knob("S##amp", &ampSustain, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpSustain] = ampSustain;
                    fUI->setParameterValue(kParameterAmpSustain, ampSustain);
                }
                ImGui::SameLine();
                float ampRelease = fUI->fParameters[kParameterAmpRelease];
                if (ImGuiKnobs::Knob("R##amp", &ampRelease, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAmpRelease] = ampRelease;
                    fUI->setParameterValue(kParameterAmpRelease, ampRelease);
                }

                ImGui::NextColumn();

                // === MODULATION & PERFORMANCE SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("MODULATION");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // LFO Waveform
                float lfoWaveform = fUI->fParameters[kParameterLFOWaveform];
                if (ImGuiKnobs::Knob("LFO Wave", &lfoWaveform, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOWaveform] = lfoWaveform;
                    fUI->setParameterValue(kParameterLFOWaveform, lfoWaveform);
                }

                // LFO Rate
                float lfoRate = fUI->fParameters[kParameterLFORate];
                if (ImGuiKnobs::Knob("LFO Rate", &lfoRate, 0.1f, 20.0f, 0.01f,
                                    "%.1f Hz", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFORate] = lfoRate;
                    fUI->setParameterValue(kParameterLFORate, lfoRate);
                }

                // LFO Pitch Depth
                float lfoPitch = fUI->fParameters[kParameterLFOPitchDepth];
                if (ImGuiKnobs::Knob("LFO Pitch", &lfoPitch, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOPitchDepth] = lfoPitch;
                    fUI->setParameterValue(kParameterLFOPitchDepth, lfoPitch);
                }

                // LFO Filter Depth
                float lfoFilter = fUI->fParameters[kParameterLFOFilterDepth];
                if (ImGuiKnobs::Knob("LFO Filter", &lfoFilter, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOFilterDepth] = lfoFilter;
                    fUI->setParameterValue(kParameterLFOFilterDepth, lfoFilter);
                }

                // LFO Amp Depth
                float lfoAmp = fUI->fParameters[kParameterLFOAmpDepth];
                if (ImGuiKnobs::Knob("LFO Amp", &lfoAmp, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterLFOAmpDepth] = lfoAmp;
                    fUI->setParameterValue(kParameterLFOAmpDepth, lfoAmp);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Velocity Sensitivity
                float velocity = fUI->fParameters[kParameterVelocitySensitivity];
                if (ImGuiKnobs::Knob("Velocity", &velocity, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterVelocitySensitivity] = velocity;
                    fUI->setParameterValue(kParameterVelocitySensitivity, velocity);
                }

                // Portamento
                float portamento = fUI->fParameters[kParameterPortamento];
                if (ImGuiKnobs::Knob("Portamento", &portamento, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, knobSize, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterPortamento] = portamento;
                    fUI->setParameterValue(kParameterPortamento, portamento);
                }

                // Volume
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
        RG101_SynthUI* const fUI;
    };

    RG101ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG101_SynthUI)
};

UI* createUI()
{
    return new RG101_SynthUI();
}

END_NAMESPACE_DISTRHO
