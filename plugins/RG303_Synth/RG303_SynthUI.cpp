#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG303_SynthUI : public UI
{
public:
    RG303_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults (matching DSP defaults)
        fParameters[kParameterWaveform] = 0.0f;     // Sawtooth
        fParameters[kParameterCutoff] = 0.5f;
        fParameters[kParameterResonance] = 0.5f;
        fParameters[kParameterEnvMod] = 0.5f;
        fParameters[kParameterDecay] = 0.3f;
        fParameters[kParameterAccent] = 0.0f;
        fParameters[kParameterSlideTime] = 0.1f;
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RG303ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG303_SynthUI() override
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
    friend class RG303ImGuiWidget;

    float fParameters[kParameterCount];

    class RG303ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG303ImGuiWidget(RG303_SynthUI* const ui)
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

            if (ImGui::Begin(RG303_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                ImGui::Spacing();

                // Title
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG303_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), RG303_DISPLAY_NAME);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Oscillator Section
                ImGui::Text("OSCILLATOR");
                ImGui::Spacing();

                const char* waveforms[] = { "Sawtooth", "Square" };
                int waveformIdx = (int)(fUI->fParameters[kParameterWaveform] + 0.5f);
                if (ImGui::Combo("Waveform", &waveformIdx, waveforms, 2)) {
                    fUI->fParameters[kParameterWaveform] = (float)waveformIdx;
                    fUI->setParameterValue(kParameterWaveform, (float)waveformIdx);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Filter Section
                ImGui::Text("FILTER");
                ImGui::Spacing();

                // Knobs for filter parameters - smooth continuous control
                float cutoff = fUI->fParameters[kParameterCutoff];
                if (ImGuiKnobs::Knob("Cutoff", &cutoff, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterCutoff] = cutoff;
                    fUI->setParameterValue(kParameterCutoff, cutoff);
                }

                ImGui::SameLine();

                float resonance = fUI->fParameters[kParameterResonance];
                if (ImGuiKnobs::Knob("Resonance", &resonance, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterResonance] = resonance;
                    fUI->setParameterValue(kParameterResonance, resonance);
                }

                ImGui::SameLine();

                float envmod = fUI->fParameters[kParameterEnvMod];
                if (ImGuiKnobs::Knob("Env Mod", &envmod, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterEnvMod] = envmod;
                    fUI->setParameterValue(kParameterEnvMod, envmod);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Envelope Section
                ImGui::Text("ENVELOPE");
                ImGui::Spacing();

                float decay = fUI->fParameters[kParameterDecay];
                if (ImGuiKnobs::Knob("Decay", &decay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterDecay] = decay;
                    fUI->setParameterValue(kParameterDecay, decay);
                }

                ImGui::SameLine();

                float accent = fUI->fParameters[kParameterAccent];
                if (ImGuiKnobs::Knob("Accent", &accent, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterAccent] = accent;
                    fUI->setParameterValue(kParameterAccent, accent);
                }

                ImGui::SameLine();

                float slideTime = fUI->fParameters[kParameterSlideTime];
                if (ImGuiKnobs::Knob("Slide", &slideTime, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSlideTime] = slideTime;
                    fUI->setParameterValue(kParameterSlideTime, slideTime);
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
        RG303_SynthUI* const fUI;
    };

    RG303ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG303_SynthUI)
};

UI* createUI()
{
    return new RG303_SynthUI();
}

END_NAMESPACE_DISTRHO
