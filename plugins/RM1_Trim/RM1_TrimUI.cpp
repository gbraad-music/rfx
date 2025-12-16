/*
 * RM1_Trim Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RM1_TrimUI : public UI
{
public:
    RM1_TrimUI()
        : UI(150, 250)
    {
        setGeometryConstraints(150, 250, true);
        fDrive = 0.7f;  // Unity gain at 70%
        fPeakLevel = 0.0f;

        fImGuiWidget = new RM1_TrimImGuiWidget(this);
        fImGuiWidget->setSize(150, 250);
    }

    ~RM1_TrimUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterDrive:
            fDrive = value;
            break;
        case kParameterPeakLevel:
            fPeakLevel = value;
            break;
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
    friend class RM1_TrimImGuiWidget;

    float fDrive;
    float fPeakLevel;

    class RM1_TrimImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RM1_TrimImGuiWidget(RM1_TrimUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RM1 Trim", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Knob styling matching RegrooveM1
                float knobSize = 80.0f;
                float knobCenterX = (getWidth() - knobSize) / 2;

                // TRIM/DRIVE label
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                float labelWidth = ImGui::CalcTextSize("TRIM").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("TRIM");

                labelWidth = ImGui::CalcTextSize("DRIVE").x;
                ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                ImGui::Text("DRIVE");
                ImGui::PopStyleColor();

                ImGui::SetCursorPosX(knobCenterX);
                ImGui::Dummy(ImVec2(0, 5.0f));

                // Drive knob with Tick style (red tick mark from style colors)
                ImGui::SetCursorPosX(knobCenterX);
                if (ImGuiKnobs::Knob("##drive", &fUI->fDrive, 0.0f, 1.0f, 0.001f,
                                     "", ImGuiKnobVariant_Tick, knobSize,
                                     ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                     10)) {
                    fUI->setParameterValue(kParameterDrive, fUI->fDrive);
                }

                // Range labels
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::SetCursorPosX(knobCenterX - 15);
                ImGui::Text("-18dB");
                ImGui::SameLine();
                ImGui::SetCursorPosX(knobCenterX + knobSize - 25);
                ImGui::Text("+6dB");
                ImGui::PopStyleColor();

                // Drive indicator LED
                ImGui::Spacing();
                ImGui::Dummy(ImVec2(0, 10.0f));

                ImVec2 p = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                float led_radius = 8.0f;
                ImVec2 led_pos = ImVec2(p.x + getWidth() / 2, p.y + led_radius + 5);

                // LED glows based on ACTUAL AUDIO peak level
                // Starts glowing at 0.5 (-6dB), full red at clipping (1.0 = 0dB)
                float peak_threshold = 0.5f;
                float glow = (fUI->fPeakLevel - peak_threshold) / (1.0f - peak_threshold);
                glow = fmaxf(0.0f, fminf(glow, 1.0f)); // Clamp between 0 and 1

                ImU32 led_color_off = IM_COL32(100, 40, 40, 255);
                ImU32 led_color_on = IM_COL32(255, 0, 0, 255);

                // Interpolate between off and on colors
                float r = ImLerp((float)((led_color_off >> IM_COL32_R_SHIFT) & 0xFF),
                                 (float)((led_color_on >> IM_COL32_R_SHIFT) & 0xFF), glow);
                float g = ImLerp((float)((led_color_off >> IM_COL32_G_SHIFT) & 0xFF),
                                 (float)((led_color_on >> IM_COL32_G_SHIFT) & 0xFF), glow);
                float b = ImLerp((float)((led_color_off >> IM_COL32_B_SHIFT) & 0xFF),
                                 (float)((led_color_on >> IM_COL32_B_SHIFT) & 0xFF), glow);
                ImU32 led_color = IM_COL32((int)r, (int)g, (int)b, 255);

                draw_list->AddCircleFilled(led_pos, led_radius, led_color);

                // LED label
                ImGui::Dummy(ImVec2(0, led_radius * 2 + 10));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                labelWidth = ImGui::CalcTextSize("DRIVE").x;
                ImGui::SetCursorPosX((getWidth() - labelWidth) / 2);
                ImGui::Text("DRIVE");
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RM1_TrimUI* const fUI;
    };

    RM1_TrimImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_TrimUI)
};

UI* createUI()
{
    return new RM1_TrimUI();
}

END_NAMESPACE_DISTRHO
