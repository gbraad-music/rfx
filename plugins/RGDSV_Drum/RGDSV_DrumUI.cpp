#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGDSV_DrumUI : public UI
{
public:
    RGDSV_DrumUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters from plugin defaults
        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Set specific defaults
        setDrumDefaults(kParameterBDWave, 0.0f, 0.3f, 0.7f, 0.5f, 0.3f, 0.1f, 0.8f, 0.8f);
        setDrumDefaults(kParameterSDWave, 0.0f, 0.5f, 0.5f, 0.3f, 0.5f, 0.6f, 0.6f, 0.7f);
        setDrumDefaults(kParameterT1Wave, 0.0f, 0.6f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f);
        setDrumDefaults(kParameterT2Wave, 0.0f, 0.5f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f);
        setDrumDefaults(kParameterT3Wave, 0.0f, 0.4f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f);
        setDrumDefaults(kParameterT4Wave, 0.0f, 0.35f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f);
        fParameters[kParameterVolume] = 0.7f;

        fImGuiWidget = new RGDSVImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGDSV_DrumUI() override
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
    void setDrumDefaults(int base, float wave, float tone, float bend, float decay,
                         float click, float noise, float filter, float level)
    {
        fParameters[base + 0] = wave;
        fParameters[base + 1] = tone;
        fParameters[base + 2] = bend;
        fParameters[base + 3] = decay;
        fParameters[base + 4] = click;
        fParameters[base + 5] = noise;
        fParameters[base + 6] = filter;
        fParameters[base + 7] = level;
    }

    friend class RGDSVImGuiWidget;

    float fParameters[kParameterCount];

    class RGDSVImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGDSVImGuiWidget(RGDSV_DrumUI* const ui)
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

            if (ImGui::Begin(RGDSV_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(10);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGDSV_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", RGDSV_DISPLAY_NAME);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Layout: 6 drums + master = 7 columns
                const float sectionWidth = (width - 40.0f) / 7.0f;
                const float knobSize = 45.0f;

                ImGui::Columns(7, "drums", false);
                for (int i = 0; i < 7; i++) {
                    ImGui::SetColumnWidth(i, sectionWidth);
                }

                // === BASS DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Text("BASS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterBDWave, knobSize);

                ImGui::NextColumn();

                // === SNARE DRUM ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                ImGui::Text("SNARE");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterSDWave, knobSize);

                ImGui::NextColumn();

                // === TOM 1 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("TOM 1");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterT1Wave, knobSize);

                ImGui::NextColumn();

                // === TOM 2 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("TOM 2");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterT2Wave, knobSize);

                ImGui::NextColumn();

                // === TOM 3 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));
                ImGui::Text("TOM 3");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterT3Wave, knobSize);

                ImGui::NextColumn();

                // === TOM 4 ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
                ImGui::Text("TOM 4");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                DRUM_KNOBS(kParameterT4Wave, knobSize);

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
        RGDSV_DrumUI* const fUI;

        void KNOB(uint32_t param, const char* label, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, 0.0f, 1.0f, 0.001f,
                                "", ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }

        void DRUM_KNOBS(uint32_t base_param, float size)
        {
            KNOB(base_param + 0, "Wave", size);
            KNOB(base_param + 1, "Tone", size);
            KNOB(base_param + 2, "Bend", size);
            KNOB(base_param + 3, "Decay", size);
            KNOB(base_param + 4, "Click", size);
            KNOB(base_param + 5, "Noise", size);
            KNOB(base_param + 6, "Filter", size);
            KNOB(base_param + 7, "Level", size);
        }
    };

    RGDSVImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGDSV_DrumUI)
};

UI* createUI()
{
    return new RGDSV_DrumUI();
}

END_NAMESPACE_DISTRHO
