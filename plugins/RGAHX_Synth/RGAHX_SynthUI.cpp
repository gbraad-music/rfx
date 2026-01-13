/*
 * RGAHX Synth UI - DearImGui Interface
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "../DearImGuiToggle/imgui_toggle.h"
#include "DistrhoPluginInfo.h"
#include <cmath>
#include <cstring>
#include <string>

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

#define RGAHX_WINDOW_TITLE "RGAHX Synth"
#define UI_WIDTH 900
#define UI_HEIGHT 600

class RGAHX_SynthUI : public UI
{
public:
    RGAHX_SynthUI() : UI(UI_WIDTH, UI_HEIGHT)
    {
        setGeometryConstraints(UI_WIDTH, UI_HEIGHT, true);

        // Initialize parameters
        for (uint32_t i = 0; i < kParameterCount; ++i) {
            fParameters[i] = 0.0f;
        }

        // Set defaults
        fParameters[kParameterWaveform] = 1.0f;  // Sawtooth
        fParameters[kParameterWaveLength] = 3.0f;
        fParameters[kParameterOscVolume] = 64.0f;
        fParameters[kParameterAttackFrames] = 1.0f;
        fParameters[kParameterAttackVolume] = 64.0f;
        fParameters[kParameterDecayFrames] = 10.0f;
        fParameters[kParameterDecayVolume] = 48.0f;
        fParameters[kParameterReleaseFrames] = 20.0f;
        fParameters[kParameterMasterVolume] = 0.7f;

        fImGuiWidget = new RGAHX_SynthImGuiWidget(this);
        fImGuiWidget->setSize(UI_WIDTH, UI_HEIGHT);

        // Initialize preset name
        strcpy(fPresetName, "Default");
    }

    ~RGAHX_SynthUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;
            fImGuiWidget->repaint();
        }
    }

    void uiIdle() override { fImGuiWidget->repaint(); }

    void uiReshape(uint width, uint height) override {
        fImGuiWidget->setSize(width, height);
    }

    void stateChanged(const char* key, const char* value) override
    {
        if (strcmp(key, "preset_name") == 0) {
            strncpy(fPresetName, value, 63);
            fPresetName[63] = '\0';
        }
    }

private:
    friend class RGAHX_SynthImGuiWidget;
    float fParameters[kParameterCount];
    char fPresetName[64];

    class RGAHX_SynthImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGAHX_SynthImGuiWidget(RGAHX_SynthUI* const ui)
            : ImGuiSubWidget(ui), fUI(ui), showPresetBrowser(false) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoScrollbar;

            if (ImGui::Begin(RGAHX_WINDOW_TITLE, nullptr, window_flags))
            {
                drawHeader(width);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Main content area
                ImGui::BeginChild("MainContent", ImVec2(0, -40), false);
                {
                    drawOscillatorSection(width);
                    ImGui::Spacing();

                    drawEnvelopeSection(width);
                    ImGui::Spacing();

                    drawModulationSection(width);
                }
                ImGui::EndChild();

                ImGui::Separator();
                drawFooter(width);

                ImGui::End();
            }

            // Preset browser popup
            if (showPresetBrowser) {
                drawPresetBrowser();
            }
        }

    private:
        RGAHX_SynthUI* const fUI;
        bool showPresetBrowser;

        void drawHeader(float width)
        {
            ImGui::SetCursorPosY(10);

            // Title
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::SetCursorPosX((width - ImGui::CalcTextSize("RGAHX SYNTH").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "RGAHX SYNTH");
            ImGui::PopFont();

            ImGui::SameLine();
            ImGui::SetCursorPosX(width - 150);

            // Preset button
            if (ImGui::Button("Presets", ImVec2(140, 0))) {
                showPresetBrowser = !showPresetBrowser;
            }

            // Subtitle
            ImGui::SetCursorPosX((width - ImGui::CalcTextSize("AHX/HVL Instrument Synthesizer").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "AHX/HVL Instrument Synthesizer");
        }

        void drawOscillatorSection(float /*width*/)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("OSCILLATOR");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::BeginGroup();
            {
                // Waveform selector with visual display
                const char* waveforms[] = {"Triangle", "Sawtooth", "Square", "Noise"};
                int waveform = (int)fUI->fParameters[kParameterWaveform];

                ImGui::Text("Waveform");
                if (ImGui::Combo("##waveform", &waveform, waveforms, 4)) {
                    fUI->fParameters[kParameterWaveform] = (float)waveform;
                    fUI->setParameterValue(kParameterWaveform, (float)waveform);
                }

                ImGui::SameLine(0, 20);

                // Draw mini waveform preview
                drawWaveformPreview(waveform, fUI->fParameters[kParameterWaveLength]);

                ImGui::SameLine(0, 40);
                KNOB(kParameterWaveLength, "Wave Len", 0.0f, 5.0f, 1.0f);

                ImGui::SameLine();
                KNOB(kParameterOscVolume, "Volume", 0.0f, 64.0f, 1.0f);
            }
            ImGui::EndGroup();
        }

        void drawEnvelopeSection(float width)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("ENVELOPE (ADSR)");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            float left_panel = width * 0.5f;

            ImGui::BeginGroup();
            {
                // Visual envelope display
                drawEnvelopeVisual(left_panel - 20, 120);

                ImGui::Spacing();

                // Envelope parameters
                KNOB(kParameterAttackFrames, "Att Frm", 0.0f, 255.0f, 1.0f);
                ImGui::SameLine();
                KNOB(kParameterAttackVolume, "Att Vol", 0.0f, 64.0f, 1.0f);
                ImGui::SameLine();
                KNOB(kParameterDecayFrames, "Dec Frm", 0.0f, 255.0f, 1.0f);
                ImGui::SameLine();
                KNOB(kParameterDecayVolume, "Dec Vol", 0.0f, 64.0f, 1.0f);

                KNOB(kParameterSustainFrames, "Sus Frm", 0.0f, 255.0f, 1.0f);
                ImGui::SameLine();
                KNOB(kParameterReleaseFrames, "Rel Frm", 0.0f, 255.0f, 1.0f);
                ImGui::SameLine();
                KNOB(kParameterReleaseVolume, "Rel Vol", 0.0f, 64.0f, 1.0f);

                ImGui::SameLine(0, 40);

                // Hard cut release
                bool hardCut = fUI->fParameters[kParameterHardCutRelease] >= 0.5f;
                if (ImGui::Checkbox("Hard Cut Release", &hardCut)) {
                    fUI->fParameters[kParameterHardCutRelease] = hardCut ? 1.0f : 0.0f;
                    fUI->setParameterValue(kParameterHardCutRelease, hardCut ? 1.0f : 0.0f);
                }

                if (hardCut) {
                    ImGui::SameLine();
                    KNOB(kParameterHardCutFrames, "Cut Frm", 0.0f, 7.0f, 1.0f);
                }
            }
            ImGui::EndGroup();
        }

        void drawModulationSection(float /*width*/)
        {
            ImGui::Columns(2, "modulation", true);

            // Filter column
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
                ImGui::Text("FILTER MODULATION");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                bool filterEnable = fUI->fParameters[kParameterFilterEnable] >= 0.5f;
                if (ImGui::Checkbox("Enable Filter", &filterEnable)) {
                    fUI->fParameters[kParameterFilterEnable] = filterEnable ? 1.0f : 0.0f;
                    fUI->setParameterValue(kParameterFilterEnable, filterEnable ? 1.0f : 0.0f);
                }

                if (filterEnable) {
                    KNOB(kParameterFilterLower, "Lower", 0.0f, 63.0f, 1.0f);
                    ImGui::SameLine();
                    KNOB(kParameterFilterUpper, "Upper", 0.0f, 63.0f, 1.0f);
                    ImGui::SameLine();
                    KNOB(kParameterFilterSpeed, "Speed", 0.0f, 63.0f, 1.0f);
                }
            }

            ImGui::NextColumn();

            // PWM column
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
                ImGui::Text("PWM (PULSE WIDTH MODULATION)");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                bool pwmEnable = fUI->fParameters[kParameterSquareEnable] >= 0.5f;
                if (ImGui::Checkbox("Enable PWM", &pwmEnable)) {
                    fUI->fParameters[kParameterSquareEnable] = pwmEnable ? 1.0f : 0.0f;
                    fUI->setParameterValue(kParameterSquareEnable, pwmEnable ? 1.0f : 0.0f);
                }

                if (pwmEnable) {
                    KNOB(kParameterSquareLower, "Lower", 0.0f, 255.0f, 1.0f);
                    ImGui::SameLine();
                    KNOB(kParameterSquareUpper, "Upper", 0.0f, 255.0f, 1.0f);
                    ImGui::SameLine();
                    KNOB(kParameterSquareSpeed, "Speed", 0.0f, 255.0f, 1.0f);
                }
            }

            ImGui::Columns(1);
            ImGui::Spacing();

            // Vibrato section
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("VIBRATO");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            KNOB(kParameterVibratoDelay, "Delay", 0.0f, 255.0f, 1.0f);
            ImGui::SameLine();
            KNOB(kParameterVibratoDepth, "Depth", 0.0f, 15.0f, 1.0f);
            ImGui::SameLine();
            KNOB(kParameterVibratoSpeed, "Speed", 0.0f, 255.0f, 1.0f);
        }

        void drawFooter(float width)
        {
            KNOB(kParameterMasterVolume, "Master", 0.0f, 1.0f, 0.01f);

            ImGui::SameLine();
            ImGui::SetCursorPosX(width - 400);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "8-Voice Polyphonic | 50Hz Frame Rate | Authentic AHX");
        }

        // Visual helpers
        void drawWaveformPreview(int waveform, float /*waveLength*/)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size(100, 60);

            // Background
            draw_list->AddRectFilled(canvas_pos,
                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                IM_COL32(20, 20, 20, 255));

            // Draw waveform
            const int samples = 100;
            ImU32 wave_color = IM_COL32(0, 180, 255, 255);

            for (int i = 0; i < samples - 1; i++) {
                float x1 = canvas_pos.x + (canvas_size.x * i / samples);
                float x2 = canvas_pos.x + (canvas_size.x * (i + 1) / samples);
                float y1 = canvas_pos.y + canvas_size.y * 0.5f + getSampleValue(waveform, i, samples) * canvas_size.y * 0.4f;
                float y2 = canvas_pos.y + canvas_size.y * 0.5f + getSampleValue(waveform, i + 1, samples) * canvas_size.y * 0.4f;
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), wave_color, 2.0f);
            }

            ImGui::Dummy(canvas_size);
        }

        float getSampleValue(int waveform, int index, int total)
        {
            float phase = (float)index / total;

            switch (waveform) {
                case 0: // Triangle
                    return (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
                case 1: // Sawtooth
                    return phase * 2.0f - 1.0f;
                case 2: // Square
                    return (phase < 0.5f) ? -1.0f : 1.0f;
                case 3: // Noise
                    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                default:
                    return 0.0f;
            }
        }

        void drawEnvelopeVisual(float w, float h)
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

            // Background
            draw_list->AddRectFilled(canvas_pos,
                ImVec2(canvas_pos.x + w, canvas_pos.y + h),
                IM_COL32(20, 20, 20, 255));

            // Get envelope values
            float a_frames = fUI->fParameters[kParameterAttackFrames];
            float a_vol = fUI->fParameters[kParameterAttackVolume];
            float d_frames = fUI->fParameters[kParameterDecayFrames];
            float d_vol = fUI->fParameters[kParameterDecayVolume];
            float s_frames = fUI->fParameters[kParameterSustainFrames];
            float r_frames = fUI->fParameters[kParameterReleaseFrames];
            float r_vol = fUI->fParameters[kParameterReleaseVolume];

            // Normalize frames for display
            float total_frames = a_frames + d_frames + (s_frames > 0 ? s_frames : 50) + r_frames;
            if (total_frames < 1.0f) total_frames = 1.0f;

            // Calculate points
            float x = canvas_pos.x;
            float y_base = canvas_pos.y + h;

            // Start point
            ImVec2 p0(x, y_base);

            // Attack peak
            x += (a_frames / total_frames) * w;
            ImVec2 p1(x, y_base - (a_vol / 64.0f) * h);

            // Decay to sustain
            x += (d_frames / total_frames) * w;
            ImVec2 p2(x, y_base - (d_vol / 64.0f) * h);

            // Sustain
            float s_display = s_frames > 0 ? s_frames : 50;
            x += (s_display / total_frames) * w;
            ImVec2 p3(x, y_base - (d_vol / 64.0f) * h);

            // Release
            x += (r_frames / total_frames) * w;
            ImVec2 p4(x, y_base - (r_vol / 64.0f) * h);

            // Draw envelope
            ImU32 env_color = IM_COL32(0, 255, 100, 255);
            draw_list->AddLine(p0, p1, env_color, 2.5f);
            draw_list->AddLine(p1, p2, env_color, 2.5f);
            draw_list->AddLine(p2, p3, env_color, 2.5f);
            draw_list->AddLine(p3, p4, env_color, 2.5f);

            // Labels
            ImGui::SetCursorScreenPos(ImVec2(p0.x + 5, p0.y - 15));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "A");
            ImGui::SetCursorScreenPos(ImVec2(p1.x + 5, p1.y - 15));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "D");
            ImGui::SetCursorScreenPos(ImVec2(p2.x + 5, p2.y - 15));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "S");
            ImGui::SetCursorScreenPos(ImVec2(p3.x + 5, p3.y - 15));
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "R");

            ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + h + 5));
            ImGui::Dummy(ImVec2(w, 0));
        }

        void drawPresetBrowser()
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(250, 150), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Preset Browser", &showPresetBrowser))
            {
                ImGui::Text("Presets");
                ImGui::Separator();

                // Preset list
                const char* presets[] = {
                    "Default",
                    "Bass - Classic AHX",
                    "Lead - Sawtooth",
                    "Pad - PWM",
                    "Hit - Percussion",
                    "Noise - Cymbal"
                };

                static int selected = 0;
                for (int i = 0; i < 6; i++) {
                    if (ImGui::Selectable(presets[i], selected == i)) {
                        selected = i;
                        loadPreset(i);
                    }
                }

                ImGui::Separator();

                if (ImGui::Button("Save Current", ImVec2(120, 0))) {
                    // TODO: Implement save
                    ImGui::OpenPopup("Save Preset");
                }

                ImGui::SameLine();

                if (ImGui::Button("Load from AHX", ImVec2(120, 0))) {
                    // TODO: Implement AHX file browser
                    ImGui::OpenPopup("Load AHX");
                }

                // Save preset popup
                if (ImGui::BeginPopup("Save Preset")) {
                    static char name[64] = "";
                    ImGui::InputText("Name", name, 64);
                    if (ImGui::Button("Save")) {
                        // TODO: Save preset
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::End();
            }
        }

        void loadPreset(int index)
        {
            // Load built-in presets
            switch (index) {
                case 0: // Default
                    fUI->setParameterValue(kParameterWaveform, 1.0f);  // Sawtooth
                    fUI->setParameterValue(kParameterWaveLength, 3.0f);
                    fUI->setParameterValue(kParameterFilterEnable, 0.0f);
                    fUI->setParameterValue(kParameterSquareEnable, 0.0f);
                    break;

                case 1: // Bass - Classic AHX
                    fUI->setParameterValue(kParameterWaveform, 2.0f);  // Square
                    fUI->setParameterValue(kParameterWaveLength, 5.0f);
                    fUI->setParameterValue(kParameterFilterEnable, 1.0f);
                    fUI->setParameterValue(kParameterFilterLower, 10.0f);
                    fUI->setParameterValue(kParameterFilterUpper, 40.0f);
                    fUI->setParameterValue(kParameterFilterSpeed, 3.0f);
                    fUI->setParameterValue(kParameterSquareEnable, 1.0f);
                    fUI->setParameterValue(kParameterSquareLower, 40.0f);
                    fUI->setParameterValue(kParameterSquareUpper, 200.0f);
                    fUI->setParameterValue(kParameterSquareSpeed, 6.0f);
                    break;

                case 2: // Lead - Sawtooth
                    fUI->setParameterValue(kParameterWaveform, 1.0f);  // Sawtooth
                    fUI->setParameterValue(kParameterFilterEnable, 1.0f);
                    fUI->setParameterValue(kParameterFilterLower, 25.0f);
                    fUI->setParameterValue(kParameterFilterUpper, 55.0f);
                    fUI->setParameterValue(kParameterFilterSpeed, 5.0f);
                    fUI->setParameterValue(kParameterVibratoDelay, 10.0f);
                    fUI->setParameterValue(kParameterVibratoDepth, 4.0f);
                    fUI->setParameterValue(kParameterVibratoSpeed, 30.0f);
                    break;

                case 3: // Pad - PWM
                    fUI->setParameterValue(kParameterWaveform, 2.0f);  // Square
                    fUI->setParameterValue(kParameterSquareEnable, 1.0f);
                    fUI->setParameterValue(kParameterSquareLower, 32.0f);
                    fUI->setParameterValue(kParameterSquareUpper, 224.0f);
                    fUI->setParameterValue(kParameterSquareSpeed, 8.0f);
                    fUI->setParameterValue(kParameterAttackFrames, 50.0f);
                    fUI->setParameterValue(kParameterReleaseFrames, 60.0f);
                    break;

                case 4: // Hit - Percussion
                    fUI->setParameterValue(kParameterWaveform, 3.0f);  // Noise
                    fUI->setParameterValue(kParameterHardCutRelease, 1.0f);
                    fUI->setParameterValue(kParameterHardCutFrames, 3.0f);
                    fUI->setParameterValue(kParameterFilterEnable, 1.0f);
                    fUI->setParameterValue(kParameterFilterLower, 5.0f);
                    fUI->setParameterValue(kParameterFilterUpper, 50.0f);
                    fUI->setParameterValue(kParameterFilterSpeed, 1.0f);
                    break;

                case 5: // Noise - Cymbal
                    fUI->setParameterValue(kParameterWaveform, 3.0f);  // Noise
                    fUI->setParameterValue(kParameterFilterEnable, 1.0f);
                    fUI->setParameterValue(kParameterFilterLower, 40.0f);
                    fUI->setParameterValue(kParameterFilterUpper, 60.0f);
                    fUI->setParameterValue(kParameterAttackFrames, 0.0f);
                    fUI->setParameterValue(kParameterDecayFrames, 30.0f);
                    fUI->setParameterValue(kParameterReleaseFrames, 40.0f);
                    break;
            }
        }

        void KNOB(uint32_t param, const char* label, float min, float max, float step)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, step, "",
                ImGuiKnobVariant_Tick, 50, ImGuiKnobFlags_NoInput, 10))
            {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RGAHX_SynthImGuiWidget* fImGuiWidget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGAHX_SynthUI)
};

UI* createUI() { return new RGAHX_SynthUI(); }

END_NAMESPACE_DISTRHO
