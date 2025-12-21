#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"
#include <cmath>
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGSonix_SynthUI : public UI
{
public:
    RGSonix_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
        , fSelectedWaveform(0)
        , fIsDragging(false)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters with defaults
        for (int i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Initialize waveforms with different presets
        generateSineWave(0);
        generateSawWave(1);
        generateSquareWave(2);
        generateTriangleWave(3);
        generatePulseWave(4);
        generateNoiseWave(5);
        generateSineWave(6);  // Extra slots start with sine
        generateSineWave(7);

        sendWaveformsToPlugin();

        fImGuiWidget = new RGSonixImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGSonix_SynthUI() override
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

    void generateSineWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = (float)i / WAVETABLE_SIZE;
            fWaveforms[index][i] = sinf(phase * 2.0f * M_PI);
        }
    }

    void generateSawWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[index][i] = 2.0f * ((float)i / WAVETABLE_SIZE) - 1.0f;
        }
    }

    void generateSquareWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[index][i] = (i < WAVETABLE_SIZE / 2) ? 1.0f : -1.0f;
        }
    }

    void generateTriangleWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = (float)i / WAVETABLE_SIZE;
            if (phase < 0.5f) {
                fWaveforms[index][i] = 4.0f * phase - 1.0f;
            } else {
                fWaveforms[index][i] = -4.0f * phase + 3.0f;
            }
        }
    }

    void generatePulseWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[index][i] = (i < WAVETABLE_SIZE / 4) ? 1.0f : -1.0f;
        }
    }

    void generateNoiseWave(int index)
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[index][i] = (((float)rand() / RAND_MAX) * 2.0f - 1.0f);
        }
    }

    void sendWaveformsToPlugin()
    {
        // Send all waveforms as semicolon-separated
        char buffer[NUM_WAVEFORMS * WAVETABLE_SIZE * 16];
        char* ptr = buffer;

        for (int w = 0; w < NUM_WAVEFORMS; w++) {
            for (int i = 0; i < WAVETABLE_SIZE; i++) {
                int written = std::sprintf(ptr, "%.6f", fWaveforms[w][i]);
                ptr += written;
                if (i < WAVETABLE_SIZE - 1) {
                    *ptr++ = ',';
                }
            }
            if (w < NUM_WAVEFORMS - 1) {
                *ptr++ = ';';
            }
        }
        *ptr = '\0';
        setState("waveforms", buffer);
    }

private:
    friend class RGSonixImGuiWidget;

    float fParameters[kParameterCount];
    float fWaveforms[NUM_WAVEFORMS][WAVETABLE_SIZE];
    int fSelectedWaveform;
    bool fIsDragging;

    class RGSonixImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGSonixImGuiWidget(RGSonix_SynthUI* const ui)
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

            if (ImGui::Begin(RGSONIX_WINDOW_TITLE, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
            {
                // Title
                ImGui::SetCursorPosY(5);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGSONIX_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.6f, 1.0f), "%s", RGSONIX_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();

                // === WAVEFORM SECTION ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("WAVEFORMS (Click to select, drag to draw)");
                ImGui::PopStyleColor();

                drawWaveformGrid();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === OSCILLATORS SECTION ===
                const float knobSize = 40.0f;

                ImGui::Columns(4, "oscillators", false);

                for (int osc = 0; osc < NUM_OSCILLATORS; osc++) {
                    int base = osc * 11;

                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImVec4(0.3f + osc * 0.15f, 0.7f - osc * 0.1f, 1.0f - osc * 0.15f, 1.0f));
                    ImGui::Text("OSCILLATOR %d", osc + 1);
                    ImGui::PopStyleColor();
                    ImGui::Spacing();

                    KNOB_INT(base + 0, "Wave", 0.0f, NUM_WAVEFORMS - 1, "%.0f", knobSize);
                    ImGui::SameLine();
                    KNOB(base + 1, "Level", 0.0f, 1.0f, knobSize);

                    KNOB_RANGE(base + 2, "Detune", -1.0f, 1.0f, "%.2f", knobSize);
                    ImGui::SameLine();
                    KNOB(base + 3, "Phase", 0.0f, 1.0f, knobSize);

                    ImGui::Spacing();
                    ImGui::Text("Amp Env");
                    KNOB(base + 4, "A##amp", 0.0f, 1.0f, 35.0f);
                    ImGui::SameLine();
                    KNOB(base + 5, "D##amp", 0.0f, 1.0f, 35.0f);
                    KNOB(base + 6, "S##amp", 0.0f, 1.0f, 35.0f);
                    ImGui::SameLine();
                    KNOB(base + 7, "R##amp", 0.0f, 1.0f, 35.0f);

                    ImGui::Spacing();
                    ImGui::Text("Pitch Env");
                    KNOB(base + 8, "A##pit", 0.0f, 1.0f, 35.0f);
                    ImGui::SameLine();
                    KNOB(base + 9, "D##pit", 0.0f, 1.0f, 35.0f);
                    KNOB_RANGE(base + 10, "Depth", -12.0f, 12.0f, "%.0f", knobSize);

                    ImGui::NextColumn();
                }

                ImGui::Columns(1);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === GLOBAL SECTION ===
                ImGui::Columns(3, "global", false);

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                ImGui::Text("PITCH");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB_RANGE(kParameterCoarseTune, "Coarse", -12.0f, 12.0f, "%.0f", knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterFineTune, "Fine", -1.0f, 1.0f, "%.2f", knobSize);

                ImGui::NextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.8f, 1.0f));
                ImGui::Text("VELOCITY SENS");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVelToAmp, "Amp", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterVelToPitch, "Pitch", 0.0f, 1.0f, knobSize);

                KNOB(kParameterVelToAttack, "Attack", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterVelToWave, "Wave", 0.0f, 1.0f, knobSize);

                ImGui::NextColumn();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::Text("MASTER");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterVolume, "Volume", 0.0f, 1.0f, knobSize);

                ImGui::Columns(1);
                ImGui::End();
            }
        }

    private:
        RGSonix_SynthUI* const fUI;

        void drawWaveformGrid()
        {
            const float wave_width = 140.0f;
            const float wave_height = 80.0f;
            const float spacing = 5.0f;

            for (int w = 0; w < NUM_WAVEFORMS; w++) {
                if (w > 0 && w % 4 != 0) ImGui::SameLine();

                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // Background
                ImU32 bg_color = (w == fUI->fSelectedWaveform) ?
                    IM_COL32(40, 40, 60, 255) : IM_COL32(20, 20, 20, 255);
                draw_list->AddRectFilled(canvas_pos,
                    ImVec2(canvas_pos.x + wave_width, canvas_pos.y + wave_height),
                    bg_color);

                // Draw waveform
                for (int i = 0; i < WAVETABLE_SIZE - 1; i++) {
                    float x1 = canvas_pos.x + (wave_width * i / (WAVETABLE_SIZE - 1));
                    float y1 = canvas_pos.y + wave_height * (0.5f - fUI->fWaveforms[w][i] * 0.45f);
                    float x2 = canvas_pos.x + (wave_width * (i + 1) / (WAVETABLE_SIZE - 1));
                    float y2 = canvas_pos.y + wave_height * (0.5f - fUI->fWaveforms[w][i + 1] * 0.45f);

                    draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2),
                        IM_COL32(100, 255, 150, 255), 1.5f);
                }

                // Border
                ImU32 border_color = (w == fUI->fSelectedWaveform) ?
                    IM_COL32(100, 255, 150, 255) : IM_COL32(100, 100, 100, 255);
                draw_list->AddRect(canvas_pos,
                    ImVec2(canvas_pos.x + wave_width, canvas_pos.y + wave_height),
                    border_color, 0.0f, 0, (w == fUI->fSelectedWaveform) ? 2.0f : 1.0f);

                // Wave number label
                char label[16];
                std::sprintf(label, "Wave %d", w);
                draw_list->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + 5),
                    IM_COL32(200, 200, 200, 255), label);

                // Invisible button for interaction
                char button_id[32];
                std::sprintf(button_id, "wave_%d", w);
                ImGui::SetCursorScreenPos(canvas_pos);
                ImGui::InvisibleButton(button_id, ImVec2(wave_width, wave_height));

                // Selection on click
                if (ImGui::IsItemClicked()) {
                    fUI->fSelectedWaveform = w;
                }

                // Drawing on drag
                if (fUI->fSelectedWaveform == w && ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f)) {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float rel_x = mouse_pos.x - canvas_pos.x;
                    float rel_y = mouse_pos.y - canvas_pos.y;

                    if (rel_x >= 0 && rel_x <= wave_width &&
                        rel_y >= 0 && rel_y <= wave_height) {

                        int idx = (int)((rel_x / wave_width) * WAVETABLE_SIZE);
                        if (idx >= 0 && idx < WAVETABLE_SIZE) {
                            float value = (0.5f - rel_y / wave_height) * (1.0f / 0.45f);
                            if (value > 1.0f) value = 1.0f;
                            if (value < -1.0f) value = -1.0f;

                            fUI->fWaveforms[w][idx] = value;
                            fUI->fIsDragging = true;
                        }
                    }
                }
            }

            if (fUI->fIsDragging && !ImGui::IsMouseDragging(0)) {
                fUI->fIsDragging = false;
                fUI->sendWaveformsToPlugin();
            }

            // Preset buttons
            ImGui::Spacing();
            ImGui::Text("Presets:");
            ImGui::SameLine();
            if (ImGui::Button("Sine")) {
                fUI->generateSineWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
            ImGui::SameLine();
            if (ImGui::Button("Saw")) {
                fUI->generateSawWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
            ImGui::SameLine();
            if (ImGui::Button("Square")) {
                fUI->generateSquareWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
            ImGui::SameLine();
            if (ImGui::Button("Triangle")) {
                fUI->generateTriangleWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
            ImGui::SameLine();
            if (ImGui::Button("Pulse")) {
                fUI->generatePulseWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
            ImGui::SameLine();
            if (ImGui::Button("Noise")) {
                fUI->generateNoiseWave(fUI->fSelectedWaveform);
                fUI->sendWaveformsToPlugin();
            }
        }

        void KNOB(uint32_t param, const char* label, float min, float max, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.001f,
                                "", ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }

        void KNOB_RANGE(uint32_t param, const char* label, float min, float max, const char* format, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 0.01f,
                                format, ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }

        void KNOB_INT(uint32_t param, const char* label, float min, float max, const char* format, float size)
        {
            float value = fUI->fParameters[param];
            if (ImGuiKnobs::Knob(label, &value, min, max, 1.0f,
                                format, ImGuiKnobVariant_Tick, size, ImGuiKnobFlags_NoInput, 10)) {
                fUI->fParameters[param] = value;
                fUI->setParameterValue(param, value);
            }
        }
    };

    RGSonixImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSonix_SynthUI)
};

UI* createUI()
{
    return new RGSonix_SynthUI();
}

END_NAMESPACE_DISTRHO
