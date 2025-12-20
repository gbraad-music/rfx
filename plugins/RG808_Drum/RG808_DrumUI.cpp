#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG808_DrumUI : public UI
{
public:
    RG808_DrumUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults (matching DSP defaults - reduced for proper mix levels)
        fParameters[kParameterKickLevel] = 0.5f;
        fParameters[kParameterKickTune] = 0.5f;
        fParameters[kParameterKickDecay] = 0.5f;
        fParameters[kParameterSnareLevel] = 0.5f;
        fParameters[kParameterSnareTune] = 0.5f;
        fParameters[kParameterSnareSnappy] = 0.5f;
        fParameters[kParameterHiHatLevel] = 0.4f;
        fParameters[kParameterHiHatDecay] = 0.3f;
        fParameters[kParameterClapLevel] = 0.5f;
        fParameters[kParameterTomLevel] = 0.5f;
        fParameters[kParameterTomTune] = 0.5f;
        fParameters[kParameterMasterLevel] = 0.5f;

        fImGuiWidget = new RG808ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG808_DrumUI() override
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
    friend class RG808ImGuiWidget;

    float fParameters[kParameterCount];

    class RG808ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG808ImGuiWidget(RG808_DrumUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG808_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                // Title
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG808_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), RG808_DISPLAY_NAME);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Kick Section
                ImGui::Text("KICK");
                ImGui::Spacing();

                float kickLevel = fUI->fParameters[kParameterKickLevel];
                if (ImGuiKnobs::Knob("Level##kick", &kickLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterKickLevel] = kickLevel;
                    fUI->setParameterValue(kParameterKickLevel, kickLevel);
                }

                ImGui::SameLine();

                float kickTune = fUI->fParameters[kParameterKickTune];
                if (ImGuiKnobs::Knob("Tune##kick", &kickTune, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterKickTune] = kickTune;
                    fUI->setParameterValue(kParameterKickTune, kickTune);
                }

                ImGui::SameLine();

                float kickDecay = fUI->fParameters[kParameterKickDecay];
                if (ImGuiKnobs::Knob("Decay##kick", &kickDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterKickDecay] = kickDecay;
                    fUI->setParameterValue(kParameterKickDecay, kickDecay);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Snare Section
                ImGui::Text("SNARE");
                ImGui::Spacing();

                float snareLevel = fUI->fParameters[kParameterSnareLevel];
                if (ImGuiKnobs::Knob("Level##snare", &snareLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSnareLevel] = snareLevel;
                    fUI->setParameterValue(kParameterSnareLevel, snareLevel);
                }

                ImGui::SameLine();

                float snareTune = fUI->fParameters[kParameterSnareTune];
                if (ImGuiKnobs::Knob("Tune##snare", &snareTune, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSnareTune] = snareTune;
                    fUI->setParameterValue(kParameterSnareTune, snareTune);
                }

                ImGui::SameLine();

                float snareSnappy = fUI->fParameters[kParameterSnareSnappy];
                if (ImGuiKnobs::Knob("Snappy##snare", &snareSnappy, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterSnareSnappy] = snareSnappy;
                    fUI->setParameterValue(kParameterSnareSnappy, snareSnappy);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Hi-Hat and Percussion Section
                ImGui::Text("HI-HAT / PERCUSSION");
                ImGui::Spacing();

                float hihatLevel = fUI->fParameters[kParameterHiHatLevel];
                if (ImGuiKnobs::Knob("HH Level", &hihatLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterHiHatLevel] = hihatLevel;
                    fUI->setParameterValue(kParameterHiHatLevel, hihatLevel);
                }

                ImGui::SameLine();

                float hihatDecay = fUI->fParameters[kParameterHiHatDecay];
                if (ImGuiKnobs::Knob("HH Decay", &hihatDecay, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterHiHatDecay] = hihatDecay;
                    fUI->setParameterValue(kParameterHiHatDecay, hihatDecay);
                }

                ImGui::SameLine();

                float clapLevel = fUI->fParameters[kParameterClapLevel];
                if (ImGuiKnobs::Knob("Clap", &clapLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterClapLevel] = clapLevel;
                    fUI->setParameterValue(kParameterClapLevel, clapLevel);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Tom Section
                ImGui::Text("TOMS");
                ImGui::Spacing();

                float tomLevel = fUI->fParameters[kParameterTomLevel];
                if (ImGuiKnobs::Knob("Level##tom", &tomLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterTomLevel] = tomLevel;
                    fUI->setParameterValue(kParameterTomLevel, tomLevel);
                }

                ImGui::SameLine();

                float tomTune = fUI->fParameters[kParameterTomTune];
                if (ImGuiKnobs::Knob("Tune##tom", &tomTune, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterTomTune] = tomTune;
                    fUI->setParameterValue(kParameterTomTune, tomTune);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Master Section
                ImGui::Text("MASTER");
                ImGui::Spacing();

                float masterLevel = fUI->fParameters[kParameterMasterLevel];
                if (ImGuiKnobs::Knob("Level##master", &masterLevel, 0.0f, 1.0f, 0.001f,
                                    "", ImGuiKnobVariant_Tick, 60, ImGuiKnobFlags_NoInput, 10)) {
                    fUI->fParameters[kParameterMasterLevel] = masterLevel;
                    fUI->setParameterValue(kParameterMasterLevel, masterLevel);
                }

                ImGui::Spacing();

                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        RG808_DrumUI* const fUI;
    };

    RG808ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG808_DrumUI)
};

UI* createUI()
{
    return new RG808_DrumUI();
}

END_NAMESPACE_DISTRHO
