#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG106_SynthUI : public UI
{
public:
    RG106_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults
        fParameters[kParameterPulseWidth] = 0.5f;
        fParameters[kParameterPWM] = 0.0f;
        fParameters[kParameterSubLevel] = 0.3f;
        fParameters[kParameterCutoff] = 0.5f;
        fParameters[kParameterResonance] = 0.3f;
        fParameters[kParameterEnvMod] = 0.5f;
        fParameters[kParameterLFOMod] = 0.0f;
        fParameters[kParameterKeyboardTracking] = 0.5f;
        fParameters[kParameterHPFCutoff] = 0.0f;
        fParameters[kParameterVCALevel] = 1.0f;
        fParameters[kParameterAttack] = 0.01f;
        fParameters[kParameterDecay] = 0.3f;
        fParameters[kParameterSustain] = 0.7f;
        fParameters[kParameterRelease] = 0.5f;
        fParameters[kParameterLFOWaveform] = 0.0f;
        fParameters[kParameterLFORate] = 5.0f;
        fParameters[kParameterLFODelay] = 0.0f;
        fParameters[kParameterLFOPitchDepth] = 0.0f;
        fParameters[kParameterLFOAmpDepth] = 0.0f;
        fParameters[kParameterChorusMode] = 0.0f;
        fParameters[kParameterChorusRate] = 0.8f;
        fParameters[kParameterChorusDepth] = 0.5f;
        fParameters[kParameterVelocitySensitivity] = 0.5f;
        fParameters[kParameterPortamento] = 0.0f;
        fParameters[kParameterVolume] = 0.4f;

        fImGuiWidget = new RG106ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG106_SynthUI() override
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
    friend class RG106ImGuiWidget;

    float fParameters[kParameterCount];

    class RG106ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG106ImGuiWidget(RG106_SynthUI* const ui)
            : ImGuiSubWidget(ui), fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            const float knobSize = 55.0f;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG106_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG106_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.6f, 1.0f), "%s", RG106_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 4 columns layout
                ImGui::Columns(4, "sections", false);

                // DCO SECTION
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("DCO");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterPulseWidth, "PW", 0.0f, 1.0f);
                KNOB(kParameterPWM, "PWM", 0.0f, 1.0f);
                KNOB(kParameterSubLevel, "Sub", 0.0f, 1.0f);

                ImGui::NextColumn();

                // VCF SECTION
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("VCF");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterCutoff, "Cutoff", 0.0f, 1.0f);
                KNOB(kParameterResonance, "Reso", 0.0f, 1.0f);
                KNOB(kParameterEnvMod, "Env Mod", 0.0f, 1.0f);
                KNOB(kParameterLFOMod, "LFO Mod", 0.0f, 1.0f);
                KNOB(kParameterKeyboardTracking, "Kbd Trk", 0.0f, 1.0f);
                KNOB(kParameterHPFCutoff, "HPF", 0.0f, 1.0f);

                ImGui::NextColumn();

                // ENVELOPE SECTION
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("ENVELOPE");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterAttack, "Attack", 0.0f, 1.0f);
                KNOB(kParameterDecay, "Decay", 0.0f, 1.0f);
                KNOB(kParameterSustain, "Sustain", 0.0f, 1.0f);
                KNOB(kParameterRelease, "Release", 0.0f, 1.0f);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                KNOB(kParameterLFOWaveform, "LFO Wave", 0.0f, 1.0f);
                KNOB_RANGE(kParameterLFORate, "LFO Rate", 0.1f, 20.0f, "%.1f Hz");
                KNOB(kParameterLFODelay, "LFO Delay", 0.0f, 1.0f);
                KNOB(kParameterLFOPitchDepth, "LFO Pitch", 0.0f, 1.0f);
                KNOB(kParameterLFOAmpDepth, "LFO Amp", 0.0f, 1.0f);

                ImGui::NextColumn();

                // CHORUS & MASTER SECTION
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("CHORUS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterChorusMode, "Mode", 0.0f, 1.0f);
                KNOB_RANGE(kParameterChorusRate, "Rate", 0.1f, 10.0f, "%.1f Hz");
                KNOB(kParameterChorusDepth, "Depth", 0.0f, 1.0f);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVCALevel, "VCA", 0.0f, 1.0f);
                KNOB(kParameterVelocitySensitivity, "Velocity", 0.0f, 1.0f);
                KNOB(kParameterPortamento, "Portamento", 0.0f, 1.0f);
                KNOB(kParameterVolume, "Volume", 0.0f, 1.0f);

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RG106_SynthUI* const fUI;

        void KNOB(uint32_t param, const char* label, float min, float max)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.001f,
                                "", ImGuiKnobVariant_Tick, 55, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }

        void KNOB_RANGE(uint32_t param, const char* label, float min, float max, const char* format)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.01f,
                                format, ImGuiKnobVariant_Tick, 55, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RG106ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG106_SynthUI)
};

UI* createUI()
{
    return new RG106_SynthUI();
}

END_NAMESPACE_DISTRHO
