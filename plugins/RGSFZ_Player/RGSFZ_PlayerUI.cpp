#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

#define DISTRHO_UI_DEFAULT_WIDTH 500
#define DISTRHO_UI_DEFAULT_HEIGHT 300

class RGSFZ_PlayerUI : public UI
{
public:
    RGSFZ_PlayerUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        fParameters[kParameterVolume] = 0.8f;
        fParameters[kParameterPan] = 0.0f;
        fParameters[kParameterAttack] = 0.001f;
        fParameters[kParameterDecay] = 0.5f;

        fImGuiWidget = new RGSFZ_ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGSFZ_PlayerUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;
            fImGuiWidget->repaint();
        }
    }

    void stateChanged(const char* key, const char* value) override
    {
        if (strcmp(key, "sfz_path") == 0) {
            snprintf(fSFZPath, sizeof(fSFZPath), "%s", value ? value : "");
            fImGuiWidget->repaint();
        }
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
    friend class RGSFZ_ImGuiWidget;
    float fParameters[kParameterCount];
    char fSFZPath[512] = {0};

    class RGSFZ_ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGSFZ_ImGuiWidget(RGSFZ_PlayerUI* const ui)
            : ImGuiSubWidget(ui)
            , fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin("RG SFZ Player", nullptr,
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGSFZ_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%s", RGSFZ_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Show loaded SFZ file
                if (fUI->fSFZPath[0]) {
                    ImGui::Text("Loaded: %s", fUI->fSFZPath);
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No SFZ file loaded");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Parameters
                ImGui::Text("PARAMETERS");
                ImGui::Spacing();

                float val;

                val = fUI->fParameters[kParameterVolume];
                if (ImGui::SliderFloat("Volume", &val, 0.0f, 1.0f)) {
                    fUI->setParameterValue(kParameterVolume, val);
                }

                val = fUI->fParameters[kParameterPan];
                if (ImGui::SliderFloat("Pan", &val, -1.0f, 1.0f)) {
                    fUI->setParameterValue(kParameterPan, val);
                }

                val = fUI->fParameters[kParameterAttack];
                if (ImGui::SliderFloat("Attack", &val, 0.0f, 1.0f)) {
                    fUI->setParameterValue(kParameterAttack, val);
                }

                val = fUI->fParameters[kParameterDecay];
                if (ImGui::SliderFloat("Decay", &val, 0.0f, 1.0f)) {
                    fUI->setParameterValue(kParameterDecay, val);
                }

                ImGui::End();
            }
        }

    private:
        RGSFZ_PlayerUI* const fUI;
    };

    RGSFZ_ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSFZ_PlayerUI)
};

UI* createUI() { return new RGSFZ_PlayerUI(); }

END_NAMESPACE_DISTRHO
