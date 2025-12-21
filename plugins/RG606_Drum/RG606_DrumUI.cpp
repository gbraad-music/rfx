#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG606_DrumUI : public UI
{
public:
    RG606_DrumUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        fParameters[kParameterBDLevel] = 0.7f;
        fParameters[kParameterBDTone] = 0.5f;
        fParameters[kParameterBDDecay] = 0.4f;
        fParameters[kParameterSDLevel] = 0.6f;
        fParameters[kParameterSDTone] = 0.5f;
        fParameters[kParameterSDSnappy] = 0.5f;
        fParameters[kParameterLTLevel] = 0.6f;
        fParameters[kParameterLTTuning] = 0.5f;
        fParameters[kParameterHTLevel] = 0.6f;
        fParameters[kParameterHTTuning] = 0.5f;
        fParameters[kParameterCHLevel] = 0.5f;
        fParameters[kParameterOHLevel] = 0.5f;
        fParameters[kParameterOHDecay] = 0.5f;
        fParameters[kParameterCYLevel] = 0.5f;
        fParameters[kParameterCYTone] = 0.5f;
        fParameters[kParameterMasterVolume] = 0.5f;
        fImGuiWidget = new RG606ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG606_DrumUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) { fParameters[index] = value; fImGuiWidget->repaint(); }
    }
    void uiIdle() override { fImGuiWidget->repaint(); }
    void uiReshape(uint width, uint height) override { UI::uiReshape(width, height); fImGuiWidget->setSize(width, height); }

private:
    friend class RG606ImGuiWidget;
    float fParameters[kParameterCount];

    class RG606ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG606ImGuiWidget(RG606_DrumUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG606_WINDOW_TITLE, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetCursorPosY(10);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG606_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", RG606_DISPLAY_NAME);
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                ImGui::Columns(4, "drums", false);

                // BASS DRUM
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Text("BASS DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB(kParameterBDLevel, "Level");
                KNOB(kParameterBDTone, "Tone");
                KNOB(kParameterBDDecay, "Decay");
                ImGui::NextColumn();

                // SNARE DRUM
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                ImGui::Text("SNARE DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB(kParameterSDLevel, "Level");
                KNOB(kParameterSDTone, "Tone");
                KNOB(kParameterSDSnappy, "Snappy");
                ImGui::NextColumn();

                // TOMS
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
                ImGui::Text("TOMS");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB(kParameterLTLevel, "LT Level");
                KNOB(kParameterLTTuning, "LT Tune");
                KNOB(kParameterHTLevel, "HT Level");
                KNOB(kParameterHTTuning, "HT Tune");
                ImGui::NextColumn();

                // CYMBALS
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("CYMBALS");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB(kParameterCHLevel, "CH Level");
                KNOB(kParameterOHLevel, "OH Level");
                KNOB(kParameterOHDecay, "OH Decay");
                KNOB(kParameterCYLevel, "CY Level");
                KNOB(kParameterCYTone, "CY Tone");

                ImGui::Columns(1);
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                // MASTER
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                KNOB(kParameterMasterVolume, "Volume");

                ImGui::End();
            }
        }

    private:
        RG606_DrumUI* const fUI;
        void KNOB(uint32_t param, const char* label) {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f, "", ImGuiKnobVariant_Tick, 40, ImGuiKnobFlags_NoInput, 8)) {
                fUI->fParameters[param] = value; fUI->setParameterValue(param, value);
            }
        }
    };

    RG606ImGuiWidget* fImGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG606_DrumUI)
};

UI* createUI() { return new RG606_DrumUI(); }

END_NAMESPACE_DISTRHO
