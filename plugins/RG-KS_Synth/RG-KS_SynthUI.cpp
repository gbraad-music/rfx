#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGKSSynthUI : public UI
{
public:
    RGKSSynthUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        fParameters[kParameterDamping] = 0.5f;
        fParameters[kParameterBrightness] = 0.5f;
        fParameters[kParameterStretch] = 0.0f;
        fParameters[kParameterPickPosition] = 0.5f;
        fParameters[kParameterVolume] = 0.5f;
        fImGuiWidget = new RGKSImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGKSSynthUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) { fParameters[index] = value; fImGuiWidget->repaint(); }
    }
    void uiIdle() override { fImGuiWidget->repaint(); }
    void uiReshape(uint width, uint height) override { UI::uiReshape(width, height); fImGuiWidget->setSize(width, height); }

private:
    friend class RGKSImGuiWidget;
    float fParameters[kParameterCount];

    class RGKSImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGKSImGuiWidget(RGKSSynthUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RGKS_WINDOW_TITLE, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetCursorPosY(10);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGKS_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s", RGKS_DISPLAY_NAME);
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                // Center the knobs
                float knob_spacing = 110.0f;
                float total_width = knob_spacing * 4;
                float start_x = (width - total_width) * 0.5f;

                ImGui::SetCursorPosX(start_x);
                ImGui::BeginGroup();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
                ImGui::Text("STRING PARAMETERS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                // First row
                KNOB(kParameterDamping, "Damping");
                ImGui::SameLine();
                KNOB(kParameterBrightness, "Brightness");
                ImGui::SameLine();
                KNOB(kParameterStretch, "Stretch");
                ImGui::SameLine();
                KNOB(kParameterPickPosition, "Pick Pos");

                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.5f, 1.0f));
                ImGui::Text("OUTPUT");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVolume, "Volume");

                ImGui::EndGroup();
                ImGui::End();
            }
        }

    private:
        RGKSSynthUI* const fUI;
        void KNOB(uint32_t param, const char* label) {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f, "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value; fUI->setParameterValue(param, value);
            }
        }
    };

    RGKSImGuiWidget* fImGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGKSSynthUI)
};

UI* createUI() { return new RGKSSynthUI(); }

END_NAMESPACE_DISTRHO
