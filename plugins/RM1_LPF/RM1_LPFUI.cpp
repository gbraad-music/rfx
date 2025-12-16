/*
 * RM1_LPF Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RM1_LPFUI : public UI
{
public:
    RM1_LPFUI()
        : UI(150, 200)
    {
        setGeometryConstraints(150, 200, true);
        fCutoff = 0.5f;

        fImGuiWidget = new RM1_LPFImGuiWidget(this);
        fImGuiWidget->setSize(150, 200);
    }

    ~RM1_LPFUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index == kParameterCutoff) {
            fCutoff = value;
        }
        fImGuiWidget->repaint();
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
    friend class RM1_LPFImGuiWidget;

    float fCutoff;

    class RM1_LPFImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RM1_LPFImGuiWidget(RM1_LPFUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RM1 LPF", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Knob styling matching RegrooveM1
                float knobSize = 80.0f;
                float knobCenterX = (getWidth() - knobSize) / 2;

                // LPF label
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                float labelWidth = ImGui::CalcTextSize("LPF").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("LPF");

                labelWidth = ImGui::CalcTextSize("CUTOFF").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("CUTOFF");
                ImGui::PopStyleColor();

                ImGui::SetCursorPosX(knobCenterX);
                ImGui::Dummy(ImVec2(0, 5.0f));

                // Cutoff knob with Tick style (red tick mark from style colors)
                ImGui::SetCursorPosX(knobCenterX);
                if (ImGuiKnobs::Knob("##lpf", &fUI->fCutoff, 0.0f, 1.0f, 0.001f,
                                     "", ImGuiKnobVariant_Tick, knobSize,
                                     ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                     10)) {
                    fUI->setParameterValue(kParameterCutoff, fUI->fCutoff);
                }

                // Range labels
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::SetCursorPosX(knobCenterX - 15);
                ImGui::Text("800Hz");
                ImGui::SameLine();
                ImGui::SetCursorPosX(knobCenterX + knobSize - 20);
                ImGui::Text("FLAT");
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RM1_LPFUI* const fUI;
    };

    RM1_LPFImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_LPFUI)
};

UI* createUI()
{
    return new RM1_LPFUI();
}

END_NAMESPACE_DISTRHO
