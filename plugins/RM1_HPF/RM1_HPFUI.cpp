/*
 * RM1_HPF Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"
#include "../rfx_ui_utils.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RM1_HPFUI : public UI
{
public:
    RM1_HPFUI()
        : UI(150, 200)
    {
        setGeometryConstraints(150, 200, true);
        fCutoff = 0.5f;

        fImGuiWidget = new RM1_HPFImGuiWidget(this);
        fImGuiWidget->setSize(150, 200);
    }

    ~RM1_HPFUI() override
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
    friend class RM1_HPFImGuiWidget;

    float fCutoff;

    class RM1_HPFImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RM1_HPFImGuiWidget(RM1_HPFUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            // Apply Regroove style (black background, red accent)
            RFX::UI::setupStyle();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RM1 HPF", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Knob styling matching RegrooveM1
                float knobSize = 80.0f;
                float knobCenterX = (getWidth() - knobSize) / 2;

                // HPF label
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                float labelWidth = ImGui::CalcTextSize("HPF").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("HPF");

                labelWidth = ImGui::CalcTextSize("CUTOFF").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("CUTOFF");
                ImGui::PopStyleColor();

                ImGui::SetCursorPosX(knobCenterX);
                ImGui::Dummy(ImVec2(0, 5.0f));

                // Cutoff knob with Tick style (red tick mark from style colors)
                ImGui::SetCursorPosX(knobCenterX);
                if (ImGuiKnobs::Knob("##hpf", &fUI->fCutoff, 0.0f, 1.0f, 0.001f,
                                     "", ImGuiKnobVariant_Tick, knobSize,
                                     ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                     10)) {
                    fUI->setParameterValue(kParameterCutoff, fUI->fCutoff);
                }

                // Range labels
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::SetCursorPosX(knobCenterX - 10);
                ImGui::Text("FLAT");
                ImGui::SameLine();
                ImGui::SetCursorPosX(knobCenterX + knobSize - 25);
                ImGui::Text("800Hz");
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RM1_HPFUI* const fUI;
    };

    RM1_HPFImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_HPFUI)
};

UI* createUI()
{
    return new RM1_HPFUI();
}

END_NAMESPACE_DISTRHO
