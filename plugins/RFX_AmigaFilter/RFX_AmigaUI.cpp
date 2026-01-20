/*
 * RFX_Amiga Plugin UI - DPF Wrapper
 * Uses DearImGui for proper dependency management
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../fx_amiga_filter_ui.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RFX_AmigaUI : public UI
{
public:
    RFX_AmigaUI()
        : UI(280, 360)
    {
        setGeometryConstraints(280, 360, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        fImGuiWidget = new RFX_AmigaImGuiWidget(this);
        fImGuiWidget->setSize(280, 360);
    }

    ~RFX_AmigaUI() override
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
    friend class RFX_AmigaImGuiWidget;

    float fParameters[2];

    class RFX_AmigaImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RFX_AmigaImGuiWidget(RFX_AmigaUI* const ui)
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

            if (ImGui::Begin("RFX AmigaFilter", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Dummy(ImVec2(0, 15.0f));

                const float faderWidth = 60.0f;
                const float spacing = RFX::UI::Size::Spacing;

                // Center the filter controls
                float contentWidth = faderWidth * 2 + spacing;  // Type + Mix faders
                float xOffset = (getWidth() - contentWidth) / 2.0f;
                if (xOffset > 0) {
                    ImGui::Indent(xOffset);
                }

                // Amiga Filter
                if (AmigaFilterUI::renderUI(
                    &fUI->fParameters[0],  // type
                    &fUI->fParameters[1],  // mix
                    faderWidth
                )) {
                    fUI->setParameterValue(0, fUI->fParameters[0]);
                    fUI->setParameterValue(1, fUI->fParameters[1]);
                }

                if (xOffset > 0) {
                    ImGui::Unindent(xOffset);
                }

                // Status line showing current settings
                ImGui::Dummy(ImVec2(0, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 5.0f));

                const char* amiga_types[] = {"A500 (4.9kHz)", "A500+LED (3.3kHz)", "A1200 (32kHz)", "A1200+LED (3.3kHz)"};
                int amiga_type = (int)(fUI->fParameters[0] + 0.5f);

                char status[128];
                snprintf(status, sizeof(status), "%s", amiga_types[amiga_type]);

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImVec2 textSize = ImGui::CalcTextSize(status);
                ImGui::SetCursorPosX((getWidth() - textSize.x) * 0.5f);
                ImGui::Text("%s", status);
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RFX_AmigaUI* const fUI;
    };

    RFX_AmigaImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_AmigaUI)
};

UI* createUI()
{
    return new RFX_AmigaUI();
}

END_NAMESPACE_DISTRHO
