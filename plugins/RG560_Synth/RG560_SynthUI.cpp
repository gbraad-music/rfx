#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG560_SynthUI : public UI
{
public:
    RG560_SynthUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        fParameters[kParameterAlgorithm] = 0.0f;
        fParameters[kParameterOp1Multiplier] = 1.0f; fParameters[kParameterOp1Level] = 1.0f; fParameters[kParameterOp1Attack] = 0.01f;
        fParameters[kParameterOp1Decay] = 0.3f; fParameters[kParameterOp1Sustain] = 0.7f; fParameters[kParameterOp1Release] = 0.3f; fParameters[kParameterOp1Waveform] = 0.0f;
        fParameters[kParameterOp2Multiplier] = 1.0f; fParameters[kParameterOp2Level] = 0.7f; fParameters[kParameterOp2Attack] = 0.01f;
        fParameters[kParameterOp2Decay] = 0.2f; fParameters[kParameterOp2Sustain] = 0.5f; fParameters[kParameterOp2Release] = 0.2f; fParameters[kParameterOp2Waveform] = 0.0f;
        fParameters[kParameterFeedback] = 0.0f; fParameters[kParameterDetune] = 0.0f;
        fParameters[kParameterLFORate] = 5.0f; fParameters[kParameterLFOPitchDepth] = 0.0f; fParameters[kParameterLFOAmpDepth] = 0.0f;
        fParameters[kParameterVelocitySensitivity] = 0.5f; fParameters[kParameterVolume] = 0.3f;
        fImGuiWidget = new RG560ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG560_SynthUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) { fParameters[index] = value; fImGuiWidget->repaint(); }
    }
    void uiIdle() override { fImGuiWidget->repaint(); }
    void uiReshape(uint width, uint height) override { UI::uiReshape(width, height); fImGuiWidget->setSize(width, height); }

private:
    friend class RG560ImGuiWidget;
    float fParameters[kParameterCount];

    class RG560ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG560ImGuiWidget(RG560_SynthUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG560_WINDOW_TITLE, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG560_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%s", RG560_DISPLAY_NAME);
                ImGui::PopFont();
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                ImGui::Columns(3, "sections", false);

                // OPERATOR 1
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("OPERATOR 1 (Carrier)");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB_RANGE(kParameterOp1Multiplier, "Mult##op1", 0.5f, 15.0f, "%.1fx");
                KNOB(kParameterOp1Level, "Level##op1"); KNOB(kParameterOp1Waveform, "Wave##op1");
                KNOB(kParameterOp1Attack, "A##op1"); KNOB(kParameterOp1Decay, "D##op1");
                KNOB(kParameterOp1Sustain, "S##op1"); KNOB(kParameterOp1Release, "R##op1");
                ImGui::NextColumn();

                // OPERATOR 2
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("OPERATOR 2 (Modulator)");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB_RANGE(kParameterOp2Multiplier, "Mult##op2", 0.5f, 15.0f, "%.1fx");
                KNOB(kParameterOp2Level, "Level##op2"); KNOB(kParameterOp2Waveform, "Wave##op2");
                KNOB(kParameterOp2Attack, "A##op2"); KNOB(kParameterOp2Decay, "D##op2");
                KNOB(kParameterOp2Sustain, "S##op2"); KNOB(kParameterOp2Release, "R##op2");
                ImGui::NextColumn();

                // CONTROL
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::Text("CONTROL");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // Algorithm selector (FM vs Additive)
                const char* algorithms[] = { "FM", "Additive" };
                int algoIndex = (fUI->fParameters[kParameterAlgorithm] >= 0.5f) ? 1 : 0;
                ImGui::SetNextItemWidth(120);
                if (ImGui::Combo("Algorithm", &algoIndex, algorithms, 2)) {
                    float newValue = (algoIndex == 0) ? 0.0f : 1.0f;
                    fUI->fParameters[kParameterAlgorithm] = newValue;
                    fUI->setParameterValue(kParameterAlgorithm, newValue);
                }

                KNOB(kParameterFeedback, "Feedback");
                KNOB(kParameterDetune, "Detune");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                KNOB_RANGE(kParameterLFORate, "LFO Rate", 0.1f, 20.0f, "%.1f Hz");
                KNOB(kParameterLFOPitchDepth, "LFO Pitch");
                KNOB(kParameterLFOAmpDepth, "LFO Amp");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                KNOB(kParameterVelocitySensitivity, "Velocity");
                KNOB(kParameterVolume, "Volume");

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RG560_SynthUI* const fUI;
        void KNOB(uint32_t param, const char* label) {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f, "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value; fUI->setParameterValue(param, value);
            }
        }
        void KNOB_RANGE(uint32_t param, const char* label, float min, float max, const char* format) {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.01f, format, ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value; fUI->setParameterValue(param, value);
            }
        }
    };

    RG560ImGuiWidget* fImGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG560_SynthUI)
};

UI* createUI() { return new RG560_SynthUI(); }

END_NAMESPACE_DISTRHO
