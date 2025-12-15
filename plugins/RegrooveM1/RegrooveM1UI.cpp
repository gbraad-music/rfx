#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RegrooveM1UI : public UI
{
public:
    RegrooveM1UI()
        : UI(200, 885)
    {
        setGeometryConstraints(200, 885, true);

        // Initialize parameters to defaults (matching MODEL 1 mixer)
        fLPFCutoff = 1.0f;    // FLAT (right)
        fSculptFreq = 0.5f;   // Center (1kHz)
        fSculptGain = 0.5f;   // 0dB (center)
        fHPFCutoff = 0.0f;    // FLAT (left)

        fImGuiWidget = new Model1ImGuiWidget(this);
        fImGuiWidget->setSize(200, 750);
    }

    ~RegrooveM1UI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case 0: fLPFCutoff = value; break;
        case 1: fSculptFreq = value; break;
        case 2: fSculptGain = value; break;
        case 3: fHPFCutoff = value; break;
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
    friend class Model1ImGuiWidget;

    class Model1ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit Model1ImGuiWidget(RegrooveM1UI* const ui)
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
            ImGui::SetNextWindowSize(ImVec2(200, 885));

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

            if (ImGui::Begin("RegrooveM1", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                // Title
                ImGui::SetCursorPosY(15);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));  // Gold

                float titleWidth = ImGui::CalcTextSize("MODEL 1").x;
                ImGui::SetCursorPosX((200 - titleWidth) / 2);
                ImGui::Text("MODEL 1");

                titleWidth = ImGui::CalcTextSize("CHANNEL").x;
                ImGui::SetCursorPosX((200 - titleWidth) / 2);
                ImGui::Text("CHANNEL");
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 25.0f));

                float knobSize = 110.0f;
                float knobCenterX = (200 - knobSize) / 2;

                // 1. CONTOUR (LPF) - 800Hz (left) to FLAT (right)
                ImGui::SetCursorPosX(knobCenterX);
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelWidth = ImGui::CalcTextSize("CONTOUR").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("CONTOUR");

                    labelWidth = ImGui::CalcTextSize("(LPF)").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("(LPF)");
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosX(knobCenterX);
                    ImGui::Dummy(ImVec2(0, 5.0f));

                    ImGui::SetCursorPosX(knobCenterX);
                    if (ImGuiKnobs::Knob("##lpf", &fUI->fLPFCutoff, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10))
                    {
                        fUI->setParameterValue(0, fUI->fLPFCutoff);
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
                ImGui::EndGroup();

                ImGui::Dummy(ImVec2(0, 30.0f));

                // 2. SCULPT FREQ - 70Hz (left) to 7kHz (right)
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
                    if (ImGuiKnobs::Knob("##sculpt_freq", &fUI->fSculptFreq, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10))
                    {
                        fUI->setParameterValue(1, fUI->fSculptFreq);
                    }

                    // Range labels
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::SetCursorPosX(knobCenterX - 10);
                    ImGui::Text("70Hz");
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(knobCenterX + knobSize - 20);
                    ImGui::Text("7kHz");
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::Dummy(ImVec2(0, 30.0f));

                // 3. SCULPT Cut/Boost - -20dB (left) to +8dB (right)
                ImGui::SetCursorPosX(knobCenterX);
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelWidth = ImGui::CalcTextSize("SCULPT").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("SCULPT");

                    labelWidth = ImGui::CalcTextSize("CUT/BOOST").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("CUT/BOOST");
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosX(knobCenterX);
                    ImGui::Dummy(ImVec2(0, 5.0f));

                    ImGui::SetCursorPosX(knobCenterX);
                    if (ImGuiKnobs::Knob("##sculpt_gain", &fUI->fSculptGain, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10))
                    {
                        fUI->setParameterValue(2, fUI->fSculptGain);
                    }

                    // Range labels
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::SetCursorPosX(knobCenterX - 15);
                    ImGui::Text("-20dB");
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(knobCenterX + knobSize - 18);
                    ImGui::Text("+8dB");
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::Dummy(ImVec2(0, 30.0f));

                // 4. CONTOUR (HPF) - FLAT (left) to 1kHz (right)
                ImGui::SetCursorPosX(knobCenterX);
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelWidth = ImGui::CalcTextSize("CONTOUR").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("CONTOUR");

                    labelWidth = ImGui::CalcTextSize("(HPF)").x;
                    ImGui::SetCursorPosX(knobCenterX + (knobSize - labelWidth) / 2);
                    ImGui::Text("(HPF)");
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosX(knobCenterX);
                    ImGui::Dummy(ImVec2(0, 5.0f));

                    ImGui::SetCursorPosX(knobCenterX);
                    if (ImGuiKnobs::Knob("##hpf", &fUI->fHPFCutoff, 0.0f, 1.0f, 0.001f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput,
                                         10))
                    {
                        fUI->setParameterValue(3, fUI->fHPFCutoff);
                    }

                    // Range labels
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::SetCursorPosX(knobCenterX - 10);
                    ImGui::Text("FLAT");
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(knobCenterX + knobSize - 20);
                    ImGui::Text("1kHz");
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();
            }
            ImGui::End();
        }

    private:
        RegrooveM1UI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Model1ImGuiWidget)
    };

    Model1ImGuiWidget* fImGuiWidget;
    float fLPFCutoff;
    float fHPFCutoff;
    float fSculptFreq;
    float fSculptGain;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveM1UI)
};

UI* createUI()
{
    return new RegrooveM1UI();
}

END_NAMESPACE_DISTRHO
