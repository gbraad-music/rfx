/*
 * RM1_Sculpt Plugin UI - DPF
 * Copyright (C) 2024
 */

#include "DistrhoUI.hpp"
#include "../rfx_imgui_wrapper.h"
#include "DearImGui/imgui.h"
#include "DearImGuiKnobs/imgui-knobs.h"

START_NAMESPACE_DISTRHO

class RM1_SculptUI : public UI,
                     public ImGuiTopLevelWidget::Callback
{
public:
    RM1_SculptUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
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

        ImGui::Begin("RM1_Sculpt", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        float freq = getParameterValue(kParameterFrequency);
        float gain = getParameterValue(kParameterGain);

        ImGui::PushID("freq_knob");
        if (ImGui::Knob("Freq", &freq, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick)) {
            setParameterValue(kParameterFrequency, freq);
        }
        ImGui::PopID();

        ImGui::Spacer();

        ImGui::PushID("gain_knob");
        if (ImGui::Knob("Gain", &gain, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick)) {
            setParameterValue(kParameterGain, gain);
        }
        ImGui::PopID();

        ImGui::End();
    }

private:
    ImGuiTopLevelWidget* imGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_SculptUI)
};

UI* createUI()
{
    return new RM1_SculptUI();
}

END_NAMESPACE_DISTRHO
