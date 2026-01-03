#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class BassStationUI : public UI
{
public:
    BassStationUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults
        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;  // Will be overridden by actual values
        }

        fImGuiWidget = new BassStationImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~BassStationUI() override
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
    friend class BassStationImGuiWidget;

    float fParameters[kParameterCount];

    class BassStationImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit BassStationImGuiWidget(BassStationUI* const ui)
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

            if (ImGui::Begin(BASS_STATION_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

                ImGui::Spacing();

                // Title
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(BASS_STATION_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "BASS STATION");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Create tabs for different sections
                if (ImGui::BeginTabBar("BassStationTabs"))
                {
                    // Oscillators Tab
                    if (ImGui::BeginTabItem("OSCILLATORS"))
                    {
                        ImGui::Columns(3, nullptr, false);

                        // OSC 1
                        ImGui::Text("OSC 1");
                        ImGui::Separator();
                        drawKnob("Waveform", kParameterOsc1Waveform, 0.0f, 3.0f, "%.0f");
                        drawKnob("Octave", kParameterOsc1Octave, -2.0f, 2.0f, "%.0f");
                        drawKnob("Fine", kParameterOsc1Fine, -12.0f, 12.0f, "%.1f");
                        drawKnob("PW", kParameterOsc1PW, 0.0f, 1.0f, "%.2f");

                        ImGui::NextColumn();

                        // OSC 2
                        ImGui::Text("OSC 2");
                        ImGui::Separator();
                        drawKnob("Waveform", kParameterOsc2Waveform, 0.0f, 3.0f, "%.0f");
                        drawKnob("Octave", kParameterOsc2Octave, -2.0f, 2.0f, "%.0f");
                        drawKnob("Fine", kParameterOsc2Fine, -12.0f, 12.0f, "%.1f");
                        drawKnob("PW", kParameterOsc2PW, 0.0f, 1.0f, "%.2f");

                        ImGui::NextColumn();

                        // Mix & Sub
                        ImGui::Text("MIX & SUB");
                        ImGui::Separator();
                        drawKnob("Mix", kParameterOscMix, 0.0f, 1.0f, "%.2f");
                        drawToggle("Sync", kParameterOscSync);
                        drawKnob("Sub Mode", kParameterSubMode, 0.0f, 2.0f, "%.0f");
                        drawKnob("Sub Wave", kParameterSubWave, 0.0f, 2.0f, "%.0f");
                        drawKnob("Sub Level", kParameterSubLevel, 0.0f, 1.0f, "%.2f");

                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    // Filter Tab
                    if (ImGui::BeginTabItem("FILTER"))
                    {
                        ImGui::Columns(2, nullptr, false);

                        ImGui::Text("FILTER");
                        ImGui::Separator();
                        drawKnob("Mode", kParameterFilterMode, 0.0f, 1.0f, "%.0f");
                        drawKnob("Type", kParameterFilterType, 0.0f, 5.0f, "%.0f");
                        drawKnob("Cutoff", kParameterFilterCutoff, 0.0f, 1.0f, "%.2f");
                        drawKnob("Resonance", kParameterFilterResonance, 0.0f, 1.0f, "%.2f");
                        drawKnob("Drive", kParameterFilterDrive, 0.0f, 1.0f, "%.2f");

                        ImGui::NextColumn();

                        ImGui::Text("MODULATION");
                        ImGui::Separator();
                        drawKnob("Env->Filter", kParameterModEnvToFilter, -1.0f, 1.0f, "%.2f");
                        drawKnob("Env->Pitch", kParameterModEnvToPitch, -1.0f, 1.0f, "%.2f");
                        drawKnob("Env->PW", kParameterModEnvToPW, -1.0f, 1.0f, "%.2f");
                        drawKnob("LFO2->Filter", kParameterLFO2ToFilter, -1.0f, 1.0f, "%.2f");
                        drawKnob("LFO2->PW", kParameterLFO2ToPW, -1.0f, 1.0f, "%.2f");

                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    // Envelopes Tab
                    if (ImGui::BeginTabItem("ENVELOPES"))
                    {
                        ImGui::Columns(2, nullptr, false);

                        // Amp Envelope
                        ImGui::Text("AMP ENVELOPE");
                        ImGui::Separator();
                        drawKnob("Attack", kParameterAmpAttack, 0.0f, 5.0f, "%.3f");
                        drawKnob("Decay", kParameterAmpDecay, 0.0f, 5.0f, "%.3f");
                        drawKnob("Sustain", kParameterAmpSustain, 0.0f, 1.0f, "%.2f");
                        drawKnob("Release", kParameterAmpRelease, 0.0f, 5.0f, "%.3f");

                        ImGui::NextColumn();

                        // Mod Envelope
                        ImGui::Text("MOD ENVELOPE");
                        ImGui::Separator();
                        drawKnob("Attack", kParameterModAttack, 0.0f, 5.0f, "%.3f");
                        drawKnob("Decay", kParameterModDecay, 0.0f, 5.0f, "%.3f");
                        drawKnob("Sustain", kParameterModSustain, 0.0f, 1.0f, "%.2f");
                        drawKnob("Release", kParameterModRelease, 0.0f, 5.0f, "%.3f");

                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    // LFO Tab
                    if (ImGui::BeginTabItem("LFOs"))
                    {
                        ImGui::Columns(2, nullptr, false);

                        // LFO 1
                        ImGui::Text("LFO 1");
                        ImGui::Separator();
                        drawKnob("Rate", kParameterLFO1Rate, 0.1f, 20.0f, "%.2f");
                        drawKnob("Waveform", kParameterLFO1Waveform, 0.0f, 5.0f, "%.0f");
                        drawKnob("To Pitch", kParameterLFO1ToPitch, -1.0f, 1.0f, "%.2f");

                        ImGui::NextColumn();

                        // LFO 2
                        ImGui::Text("LFO 2");
                        ImGui::Separator();
                        drawKnob("Rate", kParameterLFO2Rate, 0.1f, 20.0f, "%.2f");
                        drawKnob("Waveform", kParameterLFO2Waveform, 0.0f, 5.0f, "%.0f");
                        drawKnob("To PW", kParameterLFO2ToPW, -1.0f, 1.0f, "%.2f");
                        drawKnob("To Filter", kParameterLFO2ToFilter, -1.0f, 1.0f, "%.2f");

                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    // Performance Tab
                    if (ImGui::BeginTabItem("PERFORMANCE"))
                    {
                        ImGui::Columns(3, nullptr, false);

                        drawKnob("Portamento", kParameterPortamento, 0.0f, 1.0f, "%.3f");

                        ImGui::NextColumn();

                        drawKnob("Volume", kParameterVolume, 0.0f, 1.0f, "%.2f");

                        ImGui::NextColumn();

                        drawKnob("Distortion", kParameterDistortion, 0.0f, 1.0f, "%.2f");

                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

    private:
        void drawKnob(const char* label, uint32_t paramIndex, float min, float max, const char* format)
        {
            float value = fUI->fParameters[paramIndex];
            float normalizedValue = (value - min) / (max - min);

            ImGui::PushID(paramIndex);
            if (ImGuiKnobs::Knob(label, &normalizedValue, 0.0f, 1.0f, 0.001f,
                                "%.2f", ImGuiKnobVariant_Wiper)) {
                float newValue = min + normalizedValue * (max - min);
                fUI->setParameterValue(paramIndex, newValue);
            }

            // Display value
            char valueStr[64];
            std::snprintf(valueStr, sizeof(valueStr), format, value);
            ImGui::Text("%s", valueStr);
            ImGui::PopID();
        }

        void drawToggle(const char* label, uint32_t paramIndex)
        {
            bool value = fUI->fParameters[paramIndex] > 0.5f;

            ImGui::PushID(paramIndex);
            if (ImGui::Checkbox(label, &value)) {
                fUI->setParameterValue(paramIndex, value ? 1.0f : 0.0f);
            }
            ImGui::PopID();
        }

        BassStationUI* const fUI;
    };

    BassStationImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassStationUI)
};

UI* createUI()
{
    return new BassStationUI();
}

END_NAMESPACE_DISTRHO
