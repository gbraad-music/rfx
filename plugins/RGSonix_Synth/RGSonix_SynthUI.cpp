#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "../DearImGuiKnobs/imgui-knobs.h"
#include "DistrhoPluginInfo.h"
#include <cmath>
#include <cstring>

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGSonix_SynthUI : public UI
{
public:
    RGSonix_SynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters
        fParameters[kParameterAttack] = 0.01f;
        fParameters[kParameterDecay] = 0.3f;
        fParameters[kParameterSustain] = 0.7f;
        fParameters[kParameterRelease] = 0.5f;
        fParameters[kParameterPitchBend] = 0.0f;
        fParameters[kParameterFineTune] = 0.0f;
        fParameters[kParameterCoarseTune] = 0.0f;
        fParameters[kParameterVolume] = 0.7f;

        // Initialize waveform with sine wave
        generateSineWave();
        sendWaveformToPlugin();

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

    void generateSineWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = (float)i / WAVETABLE_SIZE;
            fWaveform[i] = sinf(phase * 2.0f * M_PI);
        }
    }

    void generateSawWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveform[i] = 2.0f * ((float)i / WAVETABLE_SIZE) - 1.0f;
        }
    }

    void generateSquareWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveform[i] = (i < WAVETABLE_SIZE / 2) ? 1.0f : -1.0f;
        }
    }

    void generateTriangleWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = (float)i / WAVETABLE_SIZE;
            if (phase < 0.5f) {
                fWaveform[i] = 4.0f * phase - 1.0f;
            } else {
                fWaveform[i] = -4.0f * phase + 3.0f;
            }
        }
    }

    void generatePulseWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveform[i] = (i < WAVETABLE_SIZE / 4) ? 1.0f : -1.0f;
        }
    }

    void generateNoiseWave()
    {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveform[i] = (((float)rand() / RAND_MAX) * 2.0f - 1.0f);
        }
    }

    void sendWaveformToPlugin()
    {
        // Send waveform data to plugin via state (as comma-separated values)
        char buffer[WAVETABLE_SIZE * 16];  // Large enough for all floats
        char* ptr = buffer;
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            int written = std::sprintf(ptr, "%.6f", fWaveform[i]);
            ptr += written;
            if (i < WAVETABLE_SIZE - 1) {
                *ptr++ = ',';
            }
        }
        *ptr = '\0';
        setState("waveform", buffer);
    }

private:
    friend class RGSonixImGuiWidget;

    float fParameters[kParameterCount];
    float fWaveform[WAVETABLE_SIZE];

    class RGSonixImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGSonixImGuiWidget(RGSonix_SynthUI* const ui)
            : ImGuiSubWidget(ui), fUI(ui), fIsDragging(false)
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
                ImGui::SetCursorPosY(10);
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX((width - ImGui::CalcTextSize(RGSONIX_DISPLAY_NAME).x) * 0.5f);
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.6f, 1.0f), "%s", RGSONIX_DISPLAY_NAME);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === WAVEFORM EDITOR ===
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f));
                ImGui::Text("WAVEFORM EDITOR (Click and drag to draw)");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                drawWaveformEditor();

                ImGui::Spacing();

                // Preset buttons
                ImGui::Text("Presets:");
                ImGui::SameLine();
                if (ImGui::Button("Sine")) {
                    fUI->generateSineWave();
                    fUI->sendWaveformToPlugin();
                }
                ImGui::SameLine();
                if (ImGui::Button("Saw")) {
                    fUI->generateSawWave();
                    fUI->sendWaveformToPlugin();
                }
                ImGui::SameLine();
                if (ImGui::Button("Square")) {
                    fUI->generateSquareWave();
                    fUI->sendWaveformToPlugin();
                }
                ImGui::SameLine();
                if (ImGui::Button("Triangle")) {
                    fUI->generateTriangleWave();
                    fUI->sendWaveformToPlugin();
                }
                ImGui::SameLine();
                if (ImGui::Button("Pulse")) {
                    fUI->generatePulseWave();
                    fUI->sendWaveformToPlugin();
                }
                ImGui::SameLine();
                if (ImGui::Button("Noise")) {
                    fUI->generateNoiseWave();
                    fUI->sendWaveformToPlugin();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === CONTROLS ===
                const float knobSize = 60.0f;

                ImGui::Columns(3, "controls", false);

                // Envelope
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
                ImGui::Text("ENVELOPE");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB(kParameterAttack, "Attack", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterDecay, "Decay", 0.0f, 1.0f, knobSize);

                KNOB(kParameterSustain, "Sustain", 0.0f, 1.0f, knobSize);
                ImGui::SameLine();
                KNOB(kParameterRelease, "Release", 0.0f, 1.0f, knobSize);

                ImGui::NextColumn();

                // Pitch
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("PITCH");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                KNOB_RANGE(kParameterCoarseTune, "Coarse", -12.0f, 12.0f, "%.0f", knobSize);
                ImGui::SameLine();
                KNOB_RANGE(kParameterFineTune, "Fine", -1.0f, 1.0f, "%.2f", knobSize);

                KNOB_RANGE(kParameterPitchBend, "Bend", -1.0f, 1.0f, "%.2f", knobSize);

                ImGui::NextColumn();

                // Master
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
        bool fIsDragging;

        void drawWaveformEditor()
        {
            const ImVec2 canvas_size(750, 250);
            const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Background
            draw_list->AddRectFilled(canvas_pos,
                                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                                    IM_COL32(20, 20, 20, 255));

            // Grid lines
            const int grid_lines = 8;
            for (int i = 0; i <= grid_lines; i++) {
                float y = canvas_pos.y + (canvas_size.y * i / grid_lines);
                draw_list->AddLine(ImVec2(canvas_pos.x, y),
                                  ImVec2(canvas_pos.x + canvas_size.x, y),
                                  IM_COL32(50, 50, 50, 255));
            }

            // Center line
            float center_y = canvas_pos.y + canvas_size.y * 0.5f;
            draw_list->AddLine(ImVec2(canvas_pos.x, center_y),
                              ImVec2(canvas_pos.x + canvas_size.x, center_y),
                              IM_COL32(100, 100, 100, 255), 2.0f);

            // Draw waveform
            for (int i = 0; i < WAVETABLE_SIZE - 1; i++) {
                float x1 = canvas_pos.x + (canvas_size.x * i / (WAVETABLE_SIZE - 1));
                float y1 = canvas_pos.y + canvas_size.y * (0.5f - fUI->fWaveform[i] * 0.45f);
                float x2 = canvas_pos.x + (canvas_size.x * (i + 1) / (WAVETABLE_SIZE - 1));
                float y2 = canvas_pos.y + canvas_size.y * (0.5f - fUI->fWaveform[i + 1] * 0.45f);

                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2),
                                  IM_COL32(100, 255, 150, 255), 2.0f);
            }

            // Border
            draw_list->AddRect(canvas_pos,
                              ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                              IM_COL32(150, 150, 150, 255));

            // Invisible button for mouse interaction
            ImGui::InvisibleButton("waveform_canvas", canvas_size);

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f)) {
                ImVec2 mouse_pos = ImGui::GetMousePos();
                float rel_x = mouse_pos.x - canvas_pos.x;
                float rel_y = mouse_pos.y - canvas_pos.y;

                if (rel_x >= 0 && rel_x <= canvas_size.x &&
                    rel_y >= 0 && rel_y <= canvas_size.y) {

                    int idx = (int)((rel_x / canvas_size.x) * WAVETABLE_SIZE);
                    if (idx >= 0 && idx < WAVETABLE_SIZE) {
                        // Convert mouse Y to waveform value (-1 to 1)
                        float value = (0.5f - rel_y / canvas_size.y) * (1.0f / 0.45f);
                        if (value > 1.0f) value = 1.0f;
                        if (value < -1.0f) value = -1.0f;

                        fUI->fWaveform[idx] = value;
                        fIsDragging = true;
                    }
                }
            }

            if (fIsDragging && !ImGui::IsMouseDragging(0)) {
                fIsDragging = false;
                fUI->sendWaveformToPlugin();
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
    };

    RGSonixImGuiWidget* fImGuiWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSonix_SynthUI)
};

UI* createUI()
{
    return new RGSonix_SynthUI();
}

END_NAMESPACE_DISTRHO
