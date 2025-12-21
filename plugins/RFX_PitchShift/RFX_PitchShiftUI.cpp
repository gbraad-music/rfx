#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "fx_pitchshift_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_PitchShiftUI : public UI
{
public:
    RFX_PitchShiftUI()
        : UI(380, 300)
    {
        setGeometryConstraints(380, 300, true);
        fPitch = 0.5f;     // 0 semitones
        fMix = 1.0f;       // 100% wet
        fFormant = 0.5f;   // Neutral

        fImGuiWidget = new PitchShiftImGuiWidget(this);
        fImGuiWidget->setSize(380, 300);
    }

    ~RFX_PitchShiftUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case 0: fPitch = value; break;
        case 1: fMix = value; break;
        case 2: fFormant = value; break;
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
    friend class PitchShiftImGuiWidget;

    class PitchShiftImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit PitchShiftImGuiWidget(RFX_PitchShiftUI* const ui)
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

            if (ImGui::Begin("RFX Pitch Shift", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Center the content using padding
                float contentWidth = RFX::UI::Size::FaderWidth * 3 + RFX::UI::Size::Spacing * 2;
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
                }

                if (FX::PitchShift::renderUI(&fUI->fPitch, &fUI->fMix, &fUI->fFormant, nullptr)) {
                    fUI->setParameterValue(0, fUI->fPitch);
                    fUI->setParameterValue(1, fUI->fMix);
                    fUI->setParameterValue(2, fUI->fFormant);
                }
            }
            ImGui::End();
        }

    private:
        RFX_PitchShiftUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShiftImGuiWidget)
    };

    PitchShiftImGuiWidget* fImGuiWidget;
    float fPitch;
    float fMix;
    float fFormant;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_PitchShiftUI)
};

UI* createUI()
{
    return new RFX_PitchShiftUI();
}

END_NAMESPACE_DISTRHO
