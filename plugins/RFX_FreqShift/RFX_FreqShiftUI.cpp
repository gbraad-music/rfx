#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "fx_freqshift_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_FreqShiftUI : public UI
{
public:
    RFX_FreqShiftUI()
        : UI(300, 300)
    {
        setGeometryConstraints(300, 300, true);
        fFreq = 0.5f;     // 0 Hz
        fMix = 1.0f;      // 100% wet

        fImGuiWidget = new FreqShiftImGuiWidget(this);
        fImGuiWidget->setSize(300, 300);
    }

    ~RFX_FreqShiftUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case 0: fFreq = value; break;
        case 1: fMix = value; break;
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
    friend class FreqShiftImGuiWidget;

    class FreqShiftImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit FreqShiftImGuiWidget(RFX_FreqShiftUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
            // Setup Regroove style (rounded corners, grey hover)
            RFX::UI::setupStyle();
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RFX Freq Shift", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Center the content using padding
                float contentWidth = RFX::UI::Size::FaderWidth * 2 + RFX::UI::Size::Spacing;
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
                }

                if (FX::FreqShift::renderUI(&fUI->fFreq, &fUI->fMix, nullptr)) {
                    fUI->setParameterValue(0, fUI->fFreq);
                    fUI->setParameterValue(1, fUI->fMix);
                }
            }
            ImGui::End();
        }

    private:
        RFX_FreqShiftUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreqShiftImGuiWidget)
    };

    FreqShiftImGuiWidget* fImGuiWidget;
    float fFreq;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_FreqShiftUI)
};

UI* createUI()
{
    return new RFX_FreqShiftUI();
}

END_NAMESPACE_DISTRHO
