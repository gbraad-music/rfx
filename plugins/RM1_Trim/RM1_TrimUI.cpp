/*
 * RM1_Trim Plugin UI - DPF
 * Copyright (C) 2024
 */

#include "DistrhoUI.hpp"
#include "../rfx_imgui_wrapper.h"
#include "../rfx_ui_utils.h"
#include "DearImGui/imgui.h"
#include "DearImGuiKnobs/imgui-knobs.h"

START_NAMESPACE_DISTRHO

class RM1_TrimUI : public UI,
                   public ImGuiTopLevelWidget::Callback
{
public:
    RM1_TrimUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        // Set up the ImGui wrapper
        imGuiWidget = new ImGuiTopLevelWidget(this);
        imGuiWidget->setCallback(this);
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        // Request a UI repaint when a parameter changes
        imGuiWidget->repaint();
    }

    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGui::Begin("RM1_Trim", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        float drive = getParameterValue(kParameterDrive);

        ImGui::PushID("drive_knob");
        if (ImGui::Knob("Drive", &drive, 0.0f, 1.0f, 0.01f, "%.2f", ImGuiKnobVariant_Tick)) {
            setParameterValue(kParameterDrive, drive);
        }
        ImGui::PopID();

        ImGui::End();
    }

private:
    ImGuiTopLevelWidget* imGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_TrimUI)
};

UI* createUI()
{
    return new RM1_TrimUI();
}

END_NAMESPACE_DISTRHO
