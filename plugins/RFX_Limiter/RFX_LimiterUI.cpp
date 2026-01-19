#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../fx_limiter_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_LimiterUI : public UI
{
public:
    RFX_LimiterUI()
        : UI(480, 300)
    {
        setGeometryConstraints(480, 300, true);
        fThreshold = 0.75f;
        fRelease = 0.2f;
        fCeiling = 1.0f;
        fLookahead = 0.3f;

        fImGuiWidget = new LimiterImGuiWidget(this);
        fImGuiWidget->setSize(480, 300);
    }

    ~RFX_LimiterUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case 0: fThreshold = value; break;
        case 1: fRelease = value; break;
        case 2: fCeiling = value; break;
        case 3: fLookahead = value; break;
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
    friend class LimiterImGuiWidget;

    class LimiterImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit LimiterImGuiWidget(RFX_LimiterUI* const ui)
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

            if (ImGui::Begin("RFX Limiter", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 20.0f));

                // Center the content using padding
                float contentWidth = RFX::UI::Size::FaderWidth * 4 + RFX::UI::Size::Spacing * 3;
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
                }

                if (FX::Limiter::renderUI(&fUI->fThreshold, &fUI->fRelease, &fUI->fCeiling,
                                         &fUI->fLookahead, nullptr)) {
                    fUI->setParameterValue(0, fUI->fThreshold);
                    fUI->setParameterValue(1, fUI->fRelease);
                    fUI->setParameterValue(2, fUI->fCeiling);
                    fUI->setParameterValue(3, fUI->fLookahead);
                }
            }
            ImGui::End();
        }

    private:
        RFX_LimiterUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterImGuiWidget)
    };

    LimiterImGuiWidget* fImGuiWidget;
    float fThreshold;
    float fRelease;
    float fCeiling;
    float fLookahead;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_LimiterUI)
};

UI* createUI()
{
    return new RFX_LimiterUI();
}

END_NAMESPACE_DISTRHO
