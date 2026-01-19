/*
 * RFX_Phaser Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../fx_phaser_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_PhaserUI : public UI
{
public:
    RFX_PhaserUI()
        : UI(190, 300)
    {
        setGeometryConstraints(190, 300, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_PhaserImGuiWidget(this);
        fImGuiWidget->setSize(190, 300);
    }

    ~RFX_PhaserUI() override
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
    friend class RFX_PhaserImGuiWidget;

    float fParameters[3];

    class RFX_PhaserImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_PhaserImGuiWidget(RFX_PhaserUI* const ui)
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

            if (ImGui::Begin("RFX Phaser", nullptr,
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

                if (FX::Phaser::renderUI(&fUI->fParameters[0], &fUI->fParameters[1], &fUI->fParameters[2], nullptr)) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                    fUI->setParameterValue(2, fUI->fParameters[2]);
                }
            }
            ImGui::End();
        }

    private:
        RFX_PhaserUI* const fUI;
    };

    RFX_PhaserImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_PhaserUI)
};

UI* createUI()
{
    return new RFX_PhaserUI();
}

END_NAMESPACE_DISTRHO
