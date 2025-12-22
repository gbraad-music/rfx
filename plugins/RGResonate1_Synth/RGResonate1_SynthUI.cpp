/*
 * RGResonate1 Synthesizer UI - ImGui Interface
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGResonate1_SynthUI : public UI
{
public:
    RGResonate1_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RGResonate1ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGResonate1_SynthUI() override
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
    friend class RGResonate1ImGuiWidget;

    float fParameters[kParameterCount];

    class RGResonate1ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGResonate1ImGuiWidget(RGResonate1_SynthUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin(RGRESONATE1_WINDOW_TITLE, nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoTitleBar))
            {
                // Header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                ImGui::Text("RGResonate1");
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextDisabled("Polyphonic Subtractive Synthesizer");

                ImGui::Separator();
                ImGui::Spacing();

                // Oscillator Section
                if (ImGui::CollapsingHeader("Oscillator", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    // Waveform selector
                    const char* waveforms[] = { "Saw", "Square", "Triangle", "Sine" };
                    int currentWave = (int)(fUI->fParameters[kParameterWaveform] + 0.5f);
                    if (ImGui::Combo("Waveform", &currentWave, waveforms, 4)) {
                        fUI->setParameterValue(kParameterWaveform, (float)currentWave);
                    }

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // Filter Section
                if (ImGui::CollapsingHeader("Filter", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    float cutoff = fUI->fParameters[kParameterFilterCutoff];
                    if (ImGuiKnobs::Knob("Flt Cutoff", &cutoff, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterCutoff, cutoff);
                    }
                    ImGui::SameLine();

                    float resonance = fUI->fParameters[kParameterFilterResonance];
                    if (ImGuiKnobs::Knob("Flt Resonance", &resonance, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterResonance, resonance);
                    }

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // Amplitude Envelope Section
                if (ImGui::CollapsingHeader("Amp Envelope", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    float ampAttack = fUI->fParameters[kParameterAmpAttack];
                    if (ImGuiKnobs::Knob("Amp Attack", &ampAttack, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterAmpAttack, ampAttack);
                    }
                    ImGui::SameLine();

                    float ampDecay = fUI->fParameters[kParameterAmpDecay];
                    if (ImGuiKnobs::Knob("Amp Decay", &ampDecay, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterAmpDecay, ampDecay);
                    }
                    ImGui::SameLine();

                    float ampSustain = fUI->fParameters[kParameterAmpSustain];
                    if (ImGuiKnobs::Knob("Amp Sustain", &ampSustain, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterAmpSustain, ampSustain);
                    }
                    ImGui::SameLine();

                    float ampRelease = fUI->fParameters[kParameterAmpRelease];
                    if (ImGuiKnobs::Knob("Amp Release", &ampRelease, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterAmpRelease, ampRelease);
                    }

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // Filter Envelope Section
                if (ImGui::CollapsingHeader("Filter Envelope", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    float filterEnvAmount = fUI->fParameters[kParameterFilterEnvAmount];
                    if (ImGuiKnobs::Knob("Flt Env Amount", &filterEnvAmount, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterEnvAmount, filterEnvAmount);
                    }
                    ImGui::SameLine();

                    float filterAttack = fUI->fParameters[kParameterFilterAttack];
                    if (ImGuiKnobs::Knob("Flt Attack", &filterAttack, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterAttack, filterAttack);
                    }
                    ImGui::SameLine();

                    float filterDecay = fUI->fParameters[kParameterFilterDecay];
                    if (ImGuiKnobs::Knob("Flt Decay", &filterDecay, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterDecay, filterDecay);
                    }
                    ImGui::SameLine();

                    float filterSustain = fUI->fParameters[kParameterFilterSustain];
                    if (ImGuiKnobs::Knob("Flt Sustain", &filterSustain, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterSustain, filterSustain);
                    }
                    ImGui::SameLine();

                    float filterRelease = fUI->fParameters[kParameterFilterRelease];
                    if (ImGuiKnobs::Knob("Flt Release", &filterRelease, 0.0f, 1.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                        fUI->setParameterValue(kParameterFilterRelease, filterRelease);
                    }

                    ImGui::Unindent();
                }

                ImGui::End();
            }
        }

    private:
        RGResonate1_SynthUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGResonate1ImGuiWidget)
    };

    RGResonate1ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGResonate1_SynthUI)
};

UI* createUI()
{
    return new RGResonate1_SynthUI();
}

END_NAMESPACE_DISTRHO
