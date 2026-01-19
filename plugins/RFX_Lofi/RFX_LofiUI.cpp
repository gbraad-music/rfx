/*
 * RFX_Lofi Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../fx_lofi_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_LofiUI : public UI
{
public:
    RFX_LofiUI()
        : UI(430, 320)
    {
        setGeometryConstraints(430, 320, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_LofiImGuiWidget(this);
        fImGuiWidget->setSize(430, 320);
    }

    ~RFX_LofiUI() override
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
    friend class RFX_LofiImGuiWidget;

    float fParameters[7];

    class RFX_LofiImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_LofiImGuiWidget(RFX_LofiUI* const ui)
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

            if (ImGui::Begin("RFX Lofi", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Center the content using padding (7 faders + 6 spacing gaps)
                float contentWidth = RFX::UI::Size::FaderWidth * 7 + RFX::UI::Size::Spacing * 6;
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
                }

                if (FX::Lofi::renderUI(
                    &fUI->fParameters[0],  // bit_depth
                    &fUI->fParameters[1],  // sample_rate_ratio
                    &fUI->fParameters[2],  // filter_cutoff
                    &fUI->fParameters[3],  // saturation
                    &fUI->fParameters[4],  // noise_level
                    &fUI->fParameters[5],  // wow_flutter_depth
                    &fUI->fParameters[6],  // wow_flutter_rate
                    nullptr                // enabled (not used)
                )) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                    fUI->setParameterValue(2, fUI->fParameters[2]);
                    fUI->setParameterValue(3, fUI->fParameters[3]);
                    fUI->setParameterValue(4, fUI->fParameters[4]);
                    fUI->setParameterValue(5, fUI->fParameters[5]);
                    fUI->setParameterValue(6, fUI->fParameters[6]);
                }
            }
            ImGui::End();
        }

    private:
        RFX_LofiUI* const fUI;
    };

    RFX_LofiImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_LofiUI)
};

UI* createUI()
{
    return new RFX_LofiUI();
}

END_NAMESPACE_DISTRHO
