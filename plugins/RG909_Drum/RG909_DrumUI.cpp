#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RG909_DrumUI : public UI
{
public:
    RG909_DrumUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize with defaults
        fParameters[kParameterBDLevel] = 0.8f;
        fParameters[kParameterBDTune] = 0.5f;
        fParameters[kParameterBDDecay] = 0.5f;
        fParameters[kParameterBDAttack] = 0.0f;

        fParameters[kParameterSDLevel] = 0.7f;
        fParameters[kParameterSDTone] = 0.5f;
        fParameters[kParameterSDSnappy] = 0.5f;
        fParameters[kParameterSDTuning] = 0.5f;

        fParameters[kParameterLTLevel] = 0.7f;
        fParameters[kParameterLTTuning] = 0.5f;
        fParameters[kParameterLTDecay] = 0.5f;

        fParameters[kParameterMTLevel] = 0.7f;
        fParameters[kParameterMTTuning] = 0.5f;
        fParameters[kParameterMTDecay] = 0.5f;

        fParameters[kParameterHTLevel] = 0.7f;
        fParameters[kParameterHTTuning] = 0.5f;
        fParameters[kParameterHTDecay] = 0.5f;

        fParameters[kParameterRSLevel] = 0.6f;
        fParameters[kParameterRSTuning] = 0.5f;

        fParameters[kParameterHCLevel] = 0.6f;
        fParameters[kParameterHCTone] = 0.5f;

        fParameters[kParameterMasterVolume] = 0.6f;

        fImGuiWidget = new RG909ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RG909_DrumUI() override
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
    friend class RG909ImGuiWidget;

    float fParameters[kParameterCount];

    class RG909ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RG909ImGuiWidget(RG909_DrumUI* const ui)
            : ImGuiSubWidget(ui), fUI(ui)
        {
        }

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RG909_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RG909_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", RG909_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                const float knobSize = 45.0f;

                ImGui::Columns(4, "drums", false);

                // === BASS DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Text("BASS DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterBDLevel, "Level##bd");
                KNOB(kParameterBDTune, "Tune##bd");
                KNOB(kParameterBDDecay, "Decay##bd");
                KNOB(kParameterBDAttack, "Attack##bd");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === LOW TOM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("LOW TOM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterLTLevel, "Level##lt");
                KNOB(kParameterLTTuning, "Tune##lt");
                KNOB(kParameterLTDecay, "Decay##lt");

                ImGui::NextColumn();

                // === SNARE DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::Text("SNARE DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterSDLevel, "Level##sd");
                KNOB(kParameterSDTone, "Tone##sd");
                KNOB(kParameterSDSnappy, "Snappy##sd");
                KNOB(kParameterSDTuning, "Tune##sd");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === MID TOM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("MID TOM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterMTLevel, "Level##mt");
                KNOB(kParameterMTTuning, "Tune##mt");
                KNOB(kParameterMTDecay, "Decay##mt");

                ImGui::NextColumn();

                // === RIMSHOT ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("RIMSHOT");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterRSLevel, "Level##rs");
                KNOB(kParameterRSTuning, "Tune##rs");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === HIGH TOM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("HIGH TOM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterHTLevel, "Level##ht");
                KNOB(kParameterHTTuning, "Tune##ht");
                KNOB(kParameterHTDecay, "Decay##ht");

                ImGui::NextColumn();

                // === HAND CLAP ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("HAND CLAP");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterHCLevel, "Level##hc");
                KNOB(kParameterHCTone, "Tone##hc");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === MASTER ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterMasterVolume, "Volume");

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RG909_DrumUI* const fUI;

        void KNOB(uint32_t param, const char* label)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, 45, ImGuiKnobFlags_NoInput, 8)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RG909ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG909_DrumUI)
};

UI* createUI()
{
    return new RG909_DrumUI();
}

END_NAMESPACE_DISTRHO
