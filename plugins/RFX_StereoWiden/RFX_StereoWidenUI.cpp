/*
 * RFX_StereoWiden Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "fx_stereo_widen_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_StereoWidenUI : public UI
{
public:
    RFX_StereoWidenUI()
        : UI(140, 300)
    {
        setGeometryConstraints(140, 300, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_StereoWidenImGuiWidget(this);
        fImGuiWidget->setSize(140, 300);
    }

    ~RFX_StereoWidenUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        fParameters[index] = value;
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
    friend class RFX_StereoWidenImGuiWidget;

    float fParameters[2];

    class RFX_StereoWidenImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_StereoWidenImGuiWidget(RFX_StereoWidenUI* const ui)
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

            if (ImGui::Begin("RFX Stereo Widen", nullptr,
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

                if (FX::StereoWiden::renderUI(&fUI->fParameters[0], &fUI->fParameters[1], nullptr)) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                }
            }
            ImGui::End();
        }

    private:
        RFX_StereoWidenUI* const fUI;
    };

    RFX_StereoWidenImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_StereoWidenUI)
};

UI* createUI()
{
    return new RFX_StereoWidenUI();
}

END_NAMESPACE_DISTRHO

