/*
 * RFX_Filter Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "fx_filter_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_FilterUI : public UI
{
public:
    RFX_FilterUI()
        : UI(140, 300)
    {
        setGeometryConstraints(140, 300, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_FilterImGuiWidget(this);
        fImGuiWidget->setSize(140, 300);
    }

    ~RFX_FilterUI() override
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
    friend class RFX_FilterImGuiWidget;

    float fParameters[2];

    class RFX_FilterImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_FilterImGuiWidget(RFX_FilterUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RFX Filter", nullptr,
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

                if (FX::Filter::renderUI(&fUI->fParameters[0], &fUI->fParameters[1], nullptr)) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                }
            }
            ImGui::End();
        }

    private:
        RFX_FilterUI* const fUI;
    };

    RFX_FilterImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_FilterUI)
};

UI* createUI()
{
    return new RFX_FilterUI();
}

END_NAMESPACE_DISTRHO
