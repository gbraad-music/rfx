#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../fx_compressor_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_CompressorUI : public UI
{
public:
    RFX_CompressorUI()
        : UI(340, 300)
    {
        setGeometryConstraints(340, 300, true);
        fThreshold = 0.4f;
        fRatio = 0.4f;
        fAttack = 0.05f;
        fRelease = 0.5f;
        fMakeup = 0.65f;

        fImGuiWidget = new CompressorImGuiWidget(this);
        fImGuiWidget->setSize(340, 300);
    }

    ~RFX_CompressorUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case 0: fThreshold = value; break;
        case 1: fRatio = value; break;
        case 2: fAttack = value; break;
        case 3: fRelease = value; break;
        case 4: fMakeup = value; break;
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
    friend class CompressorImGuiWidget;

    class CompressorImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit CompressorImGuiWidget(RFX_CompressorUI* const ui)
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

            if (ImGui::Begin("RFX Compressor", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Center the content using padding
                float contentWidth = RFX::UI::Size::FaderWidth * 5 + RFX::UI::Size::Spacing * 4;
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
                }

                if (FX::Compressor::renderUI(&fUI->fThreshold, &fUI->fRatio, &fUI->fAttack,
                                            &fUI->fRelease, &fUI->fMakeup, nullptr)) {
                    fUI->setParameterValue(0, fUI->fThreshold);
                    fUI->setParameterValue(1, fUI->fRatio);
                    fUI->setParameterValue(2, fUI->fAttack);
                    fUI->setParameterValue(3, fUI->fRelease);
                    fUI->setParameterValue(4, fUI->fMakeup);
                }
            }
            ImGui::End();
        }

    private:
        RFX_CompressorUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorImGuiWidget)
    };

    CompressorImGuiWidget* fImGuiWidget;
    float fThreshold;
    float fRatio;
    float fAttack;
    float fRelease;
    float fMakeup;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_CompressorUI)
};

UI* createUI()
{
    return new RFX_CompressorUI();
}

END_NAMESPACE_DISTRHO
