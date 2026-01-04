#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG1PianoUI : public UI
{
public:
    RG1PianoUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        fParameters[kParameterDecay] = 0.5f;
        fParameters[kParameterResonance] = 0.0f;
        fParameters[kParameterBrightness] = 0.6f;
        fParameters[kParameterVelocitySens] = 0.8f;
        fParameters[kParameterVolume] = 0.83f;
        fParameters[kParameterLfoRate] = 0.3f;
        fParameters[kParameterLfoDepth] = 0.2f;
        fImGuiWidget = new RG1PianoImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG1PianoUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) { fParameters[index] = value; fImGuiWidget->repaint(); }
    }
    void uiIdle() override { fImGuiWidget->repaint(); }
    void uiReshape(uint width, uint height) override { UI::uiReshape(width, height); fImGuiWidget->setSize(width, height); }

private:
    friend class RG1PianoImGuiWidget;
    float fParameters[kParameterCount];

    class RG1PianoImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG1PianoImGuiWidget(RG1PianoUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG1PIANO_WINDOW_TITLE, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG1PIANO_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%s", RG1PIANO_DISPLAY_NAME);
                ImGui::PopFont();
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                // Center the knobs
                float knob_spacing = 110.0f;
                float total_width = knob_spacing * 4;
                float start_x = (width - total_width) * 0.5f;

                ImGui::SetCursorPosX(start_x);
                ImGui::BeginGroup();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
                ImGui::Text("SYNTHESIS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // First row - Synthesis parameters
                KNOB(kParameterDecay, "Decay");
                ImGui::SameLine();
                KNOB(kParameterResonance, "Resonance");
                ImGui::SameLine();
                KNOB(kParameterBrightness, "Brightness");
                ImGui::SameLine();
                KNOB(kParameterVelocitySens, "Vel Sens");

                ImGui::Spacing(); ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
                ImGui::Text("MODULATION & OUTPUT");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // Second row - LFO and volume
                KNOB(kParameterLfoRate, "LFO Rate");
                ImGui::SameLine();
                KNOB(kParameterLfoDepth, "LFO Depth");
                ImGui::SameLine();
                KNOB(kParameterVolume, "Volume");

                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                // Info text
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                const char* info = "M1 Piano | 8-voice Polyphonic | Modal Synthesis";
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(info).x) * 0.5f);
                ImGui::Text("%s", info);
                ImGui::PopStyleColor();

                ImGui::EndGroup();
                ImGui::End();
            }
        }

    private:
        RG1PianoUI* const fUI;
        void KNOB(uint32_t param, const char* label) {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f, "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value; fUI->setParameterValue(param, value);
            }
        }
    };

    RG1PianoImGuiWidget* fImGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG1PianoUI)
};

UI* createUI() { return new RG1PianoUI(); }

END_NAMESPACE_DISTRHO
