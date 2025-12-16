/*
 * RM1_Sculpt Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RM1_SculptUI : public UI
{
public:
    RM1_SculptUI()
        : UI(150, 300)
    {
        setGeometryConstraints(150, 300, true);
        fFrequency = 0.5f;
        fGain = 0.5f;

        fImGuiWidget = new RM1_SculptImGuiWidget(this);
        fImGuiWidget->setSize(150, 300);
    }

    ~RM1_SculptUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterFrequency: fFrequency = value; break;
        case kParameterGain: fGain = value; break;
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
    friend class RM1_SculptImGuiWidget;

    float fFrequency;
    float fGain;

    class RM1_SculptImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RM1_SculptImGuiWidget(RM1_SculptUI* const ui)
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

            if (ImGui::Begin("RM1 Sculpt", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Knob styling matching RegrooveM1
                float knobSize = 80.0f;
                float knobCenterX = (getWidth() - knobSize) / 2;

                // SCULPT FREQ knob
                ImGui::SetCursorPosX(knobCenterX);
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelWidth = ImGui::CalcTextSize("SCULPT").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("SCULPT");

                    labelWidth = ImGui::CalcTextSize("FREQ").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("FREQ");
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosX(knobCenterX);
                    ImGui::Dummy(ImVec2(0, 5.0f));

                    ImGui::SetCursorPosX(knobCenterX);
                    if (ImGuiKnobs::Knob("##sculpt_freq", &fUI->fFrequency, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10)) {
                        fUI->setParameterValue(kParameterFrequency, fUI->fFrequency);
                    }

                    // Range labels
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::SetCursorPosX(knobCenterX - 10);
                    ImGui::Text("70Hz");
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(knobCenterX + knobSize - 25);
                    ImGui::Text("7kHz");
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::Dummy(ImVec2(0, 30.0f));

                // SCULPT GAIN knob
                ImGui::SetCursorPosX(knobCenterX);
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelWidth = ImGui::CalcTextSize("SCULPT").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("SCULPT");

                    labelWidth = ImGui::CalcTextSize("GAIN").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("GAIN");
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosX(knobCenterX);
                    ImGui::Dummy(ImVec2(0, 5.0f));

                    ImGui::SetCursorPosX(knobCenterX);
                    if (ImGuiKnobs::Knob("##sculpt_gain", &fUI->fGain, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10)) {
                        fUI->setParameterValue(kParameterGain, fUI->fGain);
                    }

                    // Range labels
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::SetCursorPosX(knobCenterX - 10);
                    ImGui::Text("-12dB");
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(knobCenterX + knobSize - 25);
                    ImGui::Text("+12dB");
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();
            }
            ImGui::End();
        }

    private:
        RM1_SculptUI* const fUI;
    };

    RM1_SculptImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_SculptUI)
};

UI* createUI()
{
    return new RM1_SculptUI();
}

END_NAMESPACE_DISTRHO
