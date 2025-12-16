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
        : UI(150, 200)
    {
        setGeometryConstraints(150, 200, true);
        fDrive = 0.5f;

        fImGuiWidget = new RM1_TrimImGuiWidget(this);
        fImGuiWidget->setSize(150, 200);
    }

    ~RM1_TrimUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index == kParameterDrive) {
            fDrive = value;
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
            // Model 1 color scheme matching meister icon-512x512.png
            // ButtonActive * 0.5 = outer body, so set ButtonActive to 2x target darkness
            const ImVec4 knobBody = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);       // #545454 → becomes #2a2a2a outer body
            const ImVec4 knobCenter = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);     // #8c8c8c lighter gray center cap
            const ImVec4 knobTick = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);          // PURE RED #FF0000
            const ImVec4 textColor = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);      // Light text

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            ImGuiStyle& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);  // Pure black background #000000
            style.Colors[ImGuiCol_Text] = textColor;

            // Set knob colors GLOBALLY
            style.Colors[ImGuiCol_ButtonActive] = knobBody;
            style.Colors[ImGuiCol_ButtonHovered] = knobBody;
            style.Colors[ImGuiCol_Button] = knobBody;
            style.Colors[ImGuiCol_FrameBg] = knobCenter;
            style.Colors[ImGuiCol_SliderGrab] = knobTick;
            style.Colors[ImGuiCol_SliderGrabActive] = knobTick;

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
                ImGui::SetCursorPosX(knobCenterX - 10);
                ImGui::Text("MIN");
                ImGui::SameLine();
                ImGui::SetCursorPosX(knobCenterX + knobSize - 25);
                ImGui::Text("MAX");
                ImGui::PopStyleColor();

                // Drive indicator LED
                ImGui::Spacing();
                ImGui::Dummy(ImVec2(0, 10.0f));

                ImVec2 p = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                float led_radius = 8.0f;
                ImVec2 led_pos = ImVec2(p.x + getWidth() / 2, p.y + led_radius + 5);

                // LED glows based on drive amount
                // Starts glowing at 50% drive (overdrive zone)
                float drive_threshold = 0.5f;
                float glow = (fUI->fDrive - drive_threshold) / (1.0f - drive_threshold);
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
