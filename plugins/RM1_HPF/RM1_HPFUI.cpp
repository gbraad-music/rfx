/*
 * RM1_HPF Plugin UI - DPF
 * Copyright (C) 2024
 */

#include "DistrhoUI.hpp"
#include "../rfx_imgui_wrapper.h"
#include "DearImGui/imgui.h"
#include "DearImGuiKnobs/imgui-knobs.h"

START_NAMESPACE_DISTRHO

class RM1_HPFUI : public UI,
                  public ImGuiTopLevelWidget::Callback
{
public:
    RM1_HPFUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        imGuiWidget = new ImGuiTopLevelWidget(this);
        imGuiWidget->setCallback(this);
    }

protected:
    void parameterChanged(uint32_t, float) override {
        imGuiWidget->repaint();
    }

    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGui::Begin("RM1_HPF", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        float cutoff = getParameterValue(kParameterCutoff);

        ImGui::PushID("cutoff_knob");
        if (ImGui::Knob("HPF", &cutoff, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick)) {
            setParameterValue(kParameterCutoff, cutoff);
        }
        ImGui::PopID();

        ImGui::End();
    }

private:
    ImGuiTopLevelWidget* imGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_HPFUI)
};

UI* createUI()
{
    return new RM1_HPFUI();
}

END_NAMESPACE_DISTRHO
