#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGDS3_DrumUI : public UI
{
public:
    RGDS3_DrumUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults
        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Set specific defaults
        fParameters[kParameterBDTone] = 0.3f;
        fParameters[kParameterBDBend] = 0.7f;
        fParameters[kParameterBDDecay] = 0.5f;
        fParameters[kParameterBDNoise] = 0.1f;
        fParameters[kParameterBDLevel] = 0.8f;

        fParameters[kParameterSDTone] = 0.5f;
        fParameters[kParameterSDBend] = 0.5f;
        fParameters[kParameterSDDecay] = 0.3f;
        fParameters[kParameterSDNoise] = 0.6f;
        fParameters[kParameterSDLevel] = 0.7f;

        fParameters[kParameterT1Tone] = 0.6f;
        fParameters[kParameterT1Bend] = 0.6f;
        fParameters[kParameterT1Decay] = 0.4f;
        fParameters[kParameterT1Noise] = 0.2f;
        fParameters[kParameterT1Level] = 0.7f;

        fParameters[kParameterT2Tone] = 0.5f;
        fParameters[kParameterT2Bend] = 0.6f;
        fParameters[kParameterT2Decay] = 0.4f;
        fParameters[kParameterT2Noise] = 0.2f;
        fParameters[kParameterT2Level] = 0.7f;

        fParameters[kParameterT3Tone] = 0.4f;
        fParameters[kParameterT3Bend] = 0.6f;
        fParameters[kParameterT3Decay] = 0.4f;
        fParameters[kParameterT3Noise] = 0.2f;
        fParameters[kParameterT3Level] = 0.7f;

        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RGDS3ImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGDS3_DrumUI() override
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
    friend class RGDS3ImGuiWidget;

    float fParameters[kParameterCount];

    class RGDS3ImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGDS3ImGuiWidget(RGDS3_DrumUI* const ui)
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

            if (ImGui::Begin(RGDS3_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGDS3_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", RGDS3_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Layout: 5 drum sections + master
                const float sectionWidth = (width - 40.0f) / 6.0f;
                const float knobSize = 50.0f;

                ImGui::Columns(6, "drums", false);
                for (int i = 0; i < 6; i++) {
                    ImGui::SetColumnWidth(i, sectionWidth);
                }

                // === BASS DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Text("BASS DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterBDTone, "Tone", knobSize);
                KNOB(kParameterBDBend, "Bend", knobSize);
                KNOB(kParameterBDDecay, "Decay", knobSize);
                KNOB(kParameterBDNoise, "Noise", knobSize);
                KNOB(kParameterBDLevel, "Level", knobSize);

                ImGui::NextColumn();

                // === SNARE DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                ImGui::Text("SNARE DRUM");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterSDTone, "Tone", knobSize);
                KNOB(kParameterSDBend, "Bend", knobSize);
                KNOB(kParameterSDDecay, "Decay", knobSize);
                KNOB(kParameterSDNoise, "Noise", knobSize);
                KNOB(kParameterSDLevel, "Level", knobSize);

                ImGui::NextColumn();

                // === TOM 1 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("TOM 1");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterT1Tone, "Tone", knobSize);
                KNOB(kParameterT1Bend, "Bend", knobSize);
                KNOB(kParameterT1Decay, "Decay", knobSize);
                KNOB(kParameterT1Noise, "Noise", knobSize);
                KNOB(kParameterT1Level, "Level", knobSize);

                ImGui::NextColumn();

                // === TOM 2 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("TOM 2");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterT2Tone, "Tone", knobSize);
                KNOB(kParameterT2Bend, "Bend", knobSize);
                KNOB(kParameterT2Decay, "Decay", knobSize);
                KNOB(kParameterT2Noise, "Noise", knobSize);
                KNOB(kParameterT2Level, "Level", knobSize);

                ImGui::NextColumn();

                // === TOM 3 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("TOM 3");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterT3Tone, "Tone", knobSize);
                KNOB(kParameterT3Bend, "Bend", knobSize);
                KNOB(kParameterT3Decay, "Decay", knobSize);
                KNOB(kParameterT3Noise, "Noise", knobSize);
                KNOB(kParameterT3Level, "Level", knobSize);

                ImGui::NextColumn();

                // === MASTER ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVolume, "Volume", knobSize);

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RGDS3_DrumUI* const fUI;

        void KNOB(uint32_t param, const char* label, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RGDS3ImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGDS3_DrumUI)
};

UI* createUI()
{
    return new RGDS3_DrumUI();
}

END_NAMESPACE_DISTRHO
