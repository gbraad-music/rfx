#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "imgui-knobs.h"
#include "DistrhoPluginInfo.h"
#include "../regroove_ui_helpers.hpp"

extern "C" {
#include "../../synth/rgslicer.h"
}

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGSlicerUI : public UI
{
public:
    RGSlicerUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
        , fSlicer(nullptr)
        , fNumSlices(0)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters to defaults
        for (uint32_t i = 0; i < PARAM_COUNT; i++) {
            fParameters[i] = 0.0f;
        }

        // Create temporary slicer for waveform data access
        fSlicer = rgslicer_create(48000);

        fImGuiWidget = new RGSlicerImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGSlicerUI() override {
        if (fSlicer) rgslicer_destroy(fSlicer);
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < PARAM_COUNT) {
            fParameters[index] = value;

            // Update num slices for waveform display
            if (index == PARAM_NUM_SLICES) {
                fNumSlices = (uint8_t)value;
            }

            fImGuiWidget->repaint();
        }
    }

    void stateChanged(const char* key, const char* value) override
    {
        if (std::strcmp(key, "samplePath") == 0) {
            fSamplePath = value ? value : "";

            // Load sample into UI slicer for waveform display
            if (fSlicer && fSamplePath.isNotEmpty()) {
                rgslicer_load_sample(fSlicer, fSamplePath.buffer());
                fNumSlices = rgslicer_get_num_slices(fSlicer);
            }

            fImGuiWidget->repaint();
        }
        else if (std::strcmp(key, "exportWavCue") == 0 && value && value[0]) {
            // Export WAV+CUE file
            if (fSlicer && rgslicer_has_sample(fSlicer)) {
                if (rgslicer_export_wav_cue(fSlicer, value)) {
                    printf("[RGSlicerUI] Successfully exported WAV+CUE to: %s\n", value);
                } else {
                    printf("[RGSlicerUI] Failed to export WAV+CUE\n");
                }
            }
        }
        else if (std::strcmp(key, "exportSfz") == 0 && value && value[0]) {
            // Export SFZ file
            if (fSlicer && rgslicer_has_sample(fSlicer) && fSamplePath.isNotEmpty()) {
                // For SFZ, we need to reference the original WAV file
                // Extract just the filename from the loaded sample path
                const char* wavFilename = strrchr(fSamplePath.buffer(), '/');
                if (!wavFilename) wavFilename = strrchr(fSamplePath.buffer(), '\\');
                if (wavFilename) wavFilename++;
                else wavFilename = fSamplePath.buffer();

                if (rgslicer_export_sfz(fSlicer, value, wavFilename)) {
                    printf("[RGSlicerUI] Successfully exported SFZ to: %s\n", value);
                } else {
                    printf("[RGSlicerUI] Failed to export SFZ\n");
                }
            }
        }
    }

    void uiIdle() override
    {
        // Repaint for animations
    }

private:
    friend class RGSlicerImGuiWidget;
    float fParameters[PARAM_COUNT];
    String fSamplePath;
    RGSlicer* fSlicer;
    uint8_t fNumSlices;

    class RGSlicerImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGSlicerImGuiWidget(RGSlicerUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();
            const float pad = 10.0f;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            ImGuiStyle& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_WindowBg] = RegrooveColors::BG;
            style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
            style.FrameRounding = 4.0f;
            style.WindowPadding = ImVec2(0, 0);

            // Regroove knob colors
            const ImVec4 knobBody = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);
            const ImVec4 knobCenter = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
            const ImVec4 knobTick = RegrooveColors::RED;

            style.Colors[ImGuiCol_ButtonActive] = knobBody;
            style.Colors[ImGuiCol_ButtonHovered] = knobBody;
            style.Colors[ImGuiCol_Button] = knobBody;
            style.Colors[ImGuiCol_FrameBg] = knobCenter;
            style.Colors[ImGuiCol_SliderGrab] = knobTick;
            style.Colors[ImGuiCol_SliderGrabActive] = knobTick;

            if (ImGui::Begin("RGSlicer", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
            {
                ImDrawList* draw = ImGui::GetWindowDrawList();

                // Red header
                draw->AddRectFilled(ImVec2(0, 0), ImVec2(width, 30),
                    IM_COL32(RegrooveColors::RED_R, RegrooveColors::RED_G, RegrooveColors::RED_B, 255));

                ImGui::SetCursorPosY(7);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                float tw = ImGui::CalcTextSize("RGSlicer - Slicing Sampler").x;
                ImGui::SetCursorPosX((width - tw) * 0.5f);
                ImGui::Text("RGSlicer - Slicing Sampler");
                ImGui::PopStyleColor();

                ImGui::SetCursorPosY(35);

                // ===== WAVEFORM DISPLAY WITH SLICE MARKERS =====
                ImGui::SetCursorPosX(pad);
                ImVec2 wavePos = ImGui::GetCursorScreenPos();
                const float waveW = width - 2*pad;
                const float waveH = 120.0f;

                draw->AddRectFilled(wavePos, ImVec2(wavePos.x + waveW, wavePos.y + waveH),
                    IM_COL32(0, 0, 0, 255), 4.0f);
                draw->AddRect(wavePos, ImVec2(wavePos.x + waveW, wavePos.y + waveH),
                    IM_COL32(RegrooveColors::RED_R, RegrooveColors::RED_G, RegrooveColors::RED_B, 255),
                    4.0f, 0, 2.0f);

                // Clickable area for file loading
                ImGui::SetCursorScreenPos(wavePos);
                ImGui::InvisibleButton("##waveform", ImVec2(waveW, waveH));
                if (ImGui::IsItemClicked(1)) {  // Right-click
                    fUI->requestStateFile("samplePath");
                }

                bool hasSample = fUI->fSamplePath.isNotEmpty() && fUI->fSlicer && rgslicer_has_sample(fUI->fSlicer);

                if (hasSample) {
                    // Draw waveform
                    uint32_t sampleLen = fUI->fSlicer->sample_length;
                    int16_t* sampleData = fUI->fSlicer->sample_data;

                    if (sampleData && sampleLen > 0) {
                        const float centerY = wavePos.y + waveH * 0.5f;
                        const float amp = waveH * 0.4f;

                        // Draw waveform
                        for (int x = 0; x < (int)waveW - 1; x++) {
                            uint32_t idx1 = (uint32_t)(((float)x / waveW) * sampleLen);
                            uint32_t idx2 = (uint32_t)(((float)(x + 1) / waveW) * sampleLen);
                            if (idx1 >= sampleLen) idx1 = sampleLen - 1;
                            if (idx2 >= sampleLen) idx2 = sampleLen - 1;

                            float y1 = centerY - (sampleData[idx1] / 32768.0f) * amp;
                            float y2 = centerY - (sampleData[idx2] / 32768.0f) * amp;

                            draw->AddLine(ImVec2(wavePos.x + x, y1), ImVec2(wavePos.x + x + 1, y2),
                                IM_COL32(100, 200, 100, 255), 1.0f);
                        }

                        // Draw slice markers
                        uint8_t numSlices = rgslicer_get_num_slices(fUI->fSlicer);
                        for (int i = 0; i < numSlices; i++) {
                            uint32_t offset = rgslicer_get_slice_offset(fUI->fSlicer, i);
                            float xPos = wavePos.x + (offset / (float)sampleLen) * waveW;

                            draw->AddLine(ImVec2(xPos, wavePos.y), ImVec2(xPos, wavePos.y + waveH),
                                IM_COL32(255, 255, 0, 200), 2.0f);

                            // Draw slice number
                            char sliceNum[8];
                            snprintf(sliceNum, sizeof(sliceNum), "%d", i);
                            draw->AddText(ImVec2(xPos + 2, wavePos.y + 2),
                                IM_COL32(255, 255, 0, 255), sliceNum);
                        }
                    }

                    // Filename
                    const char* p = fUI->fSamplePath.buffer();
                    const char* slash = strrchr(p, '/');
                    if (!slash) slash = strrchr(p, '\\');
                    const char* fn = slash ? (slash + 1) : p;

                    ImGui::SetCursorScreenPos(ImVec2(wavePos.x + 5, wavePos.y + waveH - 20));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::Text("%s", fn);
                    ImGui::PopStyleColor();
                } else {
                    // No sample
                    const char* msg = "Right-click to load WAV";
                    ImGui::SetCursorScreenPos(ImVec2(wavePos.x + (waveW - ImGui::CalcTextSize(msg).x) * 0.5f,
                                                      wavePos.y + waveH * 0.5f - 10));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                    ImGui::Text("%s", msg);
                    ImGui::PopStyleColor();
                }

                ImGui::SetCursorPosY(wavePos.y + waveH + pad + 5);

                // ===== EXPORT BUTTONS =====
                if (hasSample && fUI->fNumSlices > 0) {
                    ImGui::SetCursorPosX(pad);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));

                    if (ImGui::Button("Export WAV+CUE", ImVec2(140, 25))) {
                        // Request file path for export (write mode)
                        fUI->requestStateFile("exportWavCue");
                    }

                    ImGui::SameLine(0, pad);

                    if (ImGui::Button("Export SFZ", ImVec2(140, 25))) {
                        fUI->requestStateFile("exportSfz");
                    }

                    ImGui::PopStyleColor(3);

                    ImGui::Dummy(ImVec2(0, 5));
                }

                ImGui::SetCursorPosY(wavePos.y + waveH + pad + 35);

                // ===== KNOBS SECTION =====
                const float knobSize = 80.0f;
                const float knobSpacing = (width - 6 * knobSize) / 7.0f;

                ImGui::SetCursorPosX(knobSpacing);

                // Master Volume
                float vol = fUI->fParameters[PARAM_MASTER_VOLUME] / 100.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("VOL").x;
                    ImGui::SetCursorPosX(knobSpacing + (knobSize - labelW) * 0.5f);
                    ImGui::Text("VOL");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##vol", &vol, 0.0f, 2.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_MASTER_VOLUME, vol * 100.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Master Pitch
                float pitch = (fUI->fParameters[PARAM_MASTER_PITCH] + 12.0f) / 24.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("PITCH").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("PITCH");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##pitch", &pitch, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_MASTER_PITCH, pitch * 24.0f - 12.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Master Time
                float time = (fUI->fParameters[PARAM_MASTER_TIME] - 50.0f) / 150.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("TIME").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("TIME");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##time", &time, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_MASTER_TIME, time * 150.0f + 50.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Slice Mode
                float mode = fUI->fParameters[PARAM_SLICE_MODE] / 3.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("MODE").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("MODE");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##mode", &mode, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SLICE_MODE, roundf(mode * 3.0f));
                    }

                    // Mode label
                    const char* modeNames[] = { "TRANS", "ZERO", "GRID", "BPM" };
                    int modeIdx = (int)fUI->fParameters[PARAM_SLICE_MODE];
                    if (modeIdx < 0) modeIdx = 0;
                    if (modeIdx > 3) modeIdx = 3;
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    float modeLabelW = ImGui::CalcTextSize(modeNames[modeIdx]).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - modeLabelW) * 0.5f);
                    ImGui::Text("%s", modeNames[modeIdx]);
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Num Slices
                float slices = (fUI->fParameters[PARAM_NUM_SLICES] - 1.0f) / 63.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("SLICES").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("SLICES");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##slices", &slices, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_NUM_SLICES, roundf(slices * 63.0f + 1.0f));
                    }

                    // Value label
                    char sliceVal[8];
                    snprintf(sliceVal, sizeof(sliceVal), "%d", (int)fUI->fParameters[PARAM_NUM_SLICES]);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    float sliceValW = ImGui::CalcTextSize(sliceVal).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - sliceValW) * 0.5f);
                    ImGui::Text("%s", sliceVal);
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Sensitivity
                float sense = fUI->fParameters[PARAM_SENSITIVITY] / 100.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("SENSE").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("SENSE");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##sense", &sense, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SENSITIVITY, sense * 100.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::Dummy(ImVec2(0, 15));

                // ===== SLICE 0 CONTROLS =====
                ImGui::SetCursorPosX(pad);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                ImGui::Text("SLICE 0 CONTROLS");
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 5));

                ImGui::SetCursorPosX(knobSpacing);

                // S0 Pitch
                float s0pitch = (fUI->fParameters[PARAM_SLICE0_PITCH] + 12.0f) / 24.0f;
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 PITCH");
                    if (ImGuiKnobs::Knob("##s0pitch", &s0pitch, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_PITCH, s0pitch * 24.0f - 12.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // S0 Time
                float s0time = (fUI->fParameters[PARAM_SLICE0_TIME] - 50.0f) / 150.0f;
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 TIME");
                    if (ImGuiKnobs::Knob("##s0time", &s0time, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_TIME, s0time * 150.0f + 50.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // S0 Volume
                float s0vol = fUI->fParameters[PARAM_SLICE0_VOLUME] / 200.0f;
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 VOL");
                    if (ImGuiKnobs::Knob("##s0vol", &s0vol, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_VOLUME, s0vol * 200.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // S0 Pan
                float s0pan = (fUI->fParameters[PARAM_SLICE0_PAN] + 100.0f) / 200.0f;
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 PAN");
                    if (ImGuiKnobs::Knob("##s0pan", &s0pan, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_PAN, s0pan * 200.0f - 100.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::End();
            }
        }

    private:
        RGSlicerUI* const fUI;
    };

    RGSlicerImGuiWidget* fImGuiWidget;
};

UI* createUI()
{
    return new RGSlicerUI();
}

END_NAMESPACE_DISTRHO
