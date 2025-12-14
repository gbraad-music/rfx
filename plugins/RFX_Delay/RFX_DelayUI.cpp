/*
 * RFX_Delay Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "fx_delay_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_DelayUI : public UI
{
public:
    RFX_DelayUI()
        : UI(380, 320)
    {
        setGeometryConstraints(380, 320, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_DelayImGuiWidget(this);
        fImGuiWidget->setSize(380, 320);
    }

    ~RFX_DelayUI() override
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
    friend class RFX_DelayImGuiWidget;

    float fParameters[3];

    class RFX_DelayImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_DelayImGuiWidget(RFX_DelayUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            if (ImGui::Begin("RFX Delay", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                float xOffset = (getWidth() - (RFX::UI::Size::FaderWidth * 3 + RFX::UI::Size::Spacing * 2)) / 2.0f;
                ImGui::SetCursorPosX(xOffset);

                if (FX::Delay::renderUI(&fUI->fParameters[0], &fUI->fParameters[1], &fUI->fParameters[2], nullptr)) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                    fUI->setParameterValue(2, fUI->fParameters[2]);
                }
            }
            ImGui::End();
        }

    private:
        RFX_DelayUI* const fUI;
    };

    RFX_DelayImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_DelayUI)
};

UI* createUI()
{
    return new RFX_DelayUI();
}

END_NAMESPACE_DISTRHO
