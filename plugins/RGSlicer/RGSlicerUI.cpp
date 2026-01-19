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

        // Initialize parameters to defaults (matching plugin defaults)
        fParameters[PARAM_MASTER_VOLUME] = 100.0f;
        fParameters[PARAM_MASTER_PITCH] = 0.0f;
        fParameters[PARAM_MASTER_TIME] = 100.0f;
        fParameters[PARAM_SLICE0_PITCH] = 0.0f;
        fParameters[PARAM_SLICE0_TIME] = 100.0f;
        fParameters[PARAM_SLICE0_VOLUME] = 100.0f;
        fParameters[PARAM_SLICE0_PAN] = 0.0f;
        fParameters[PARAM_SLICE0_LOOP] = 0.0f;
        fParameters[PARAM_SLICE0_ONE_SHOT] = 0.0f;
        fParameters[PARAM_BPM] = 125.0f;
        fParameters[PARAM_NOTE_DIVISION] = 4.0f;  // 16th notes default

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
        // Repaint to sync UI with parameter changes from DAW/automation
        fImGuiWidget->repaint();
    }

    void uiReshape(uint width, uint height) override
    {
        UI::uiReshape(width, height);
        fImGuiWidget->setSize(width, height);
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
        explicit RGSlicerImGuiWidget(RGSlicerUI* const ui)
            : ImGuiSubWidget(ui)
            , fUI(ui)
            , fZoomLevel(1.0f)
            , fPanOffset(0.0f)
            , fZoomWheelAccum(0.0f)
            , fMarkerState(MARKER_IDLE)
            , fHoveredMarkerIndex(-1)
            , fDraggedMarkerIndex(-1)
            , fDragStartMouseX(0.0f)
            , fDragStartOffset(0)
            , fTriggerToggle(false)
        {}

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

            // All buttons use Regroove red
            style.Colors[ImGuiCol_Button] = RegrooveColors::RED;
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(RegrooveColors::RED_R/255.0f * 1.2f, RegrooveColors::RED_G/255.0f * 1.5f, RegrooveColors::RED_B/255.0f * 1.2f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(RegrooveColors::RED_R/255.0f * 1.3f, RegrooveColors::RED_G/255.0f * 1.8f, RegrooveColors::RED_B/255.0f * 1.3f, 1.0f);

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

                // Clickable area for waveform interaction
                ImGui::SetCursorScreenPos(wavePos);
                ImGui::InvisibleButton("##waveform", ImVec2(waveW, waveH));

                bool isWaveformHovered = ImGui::IsItemHovered();
                bool isWaveformActive = ImGui::IsItemActive();
                bool leftClicked = ImGui::IsItemClicked(0);
                bool rightClicked = ImGui::IsItemClicked(1);
                ImGuiIO& io = ImGui::GetIO();

                // === ZOOM with Mouse Wheel ===
                if (isWaveformHovered && io.MouseWheel != 0.0f && !isWaveformActive) {
                    fZoomWheelAccum += io.MouseWheel;

                    if (fabs(fZoomWheelAccum) >= 0.5f) {
                        ImVec2 mousePos = ImGui::GetMousePos();
                        float mouseRelX = (mousePos.x - wavePos.x) / waveW;

                        // Get sample currently under mouse
                        uint32_t sampleLen = fUI->fSlicer ? fUI->fSlicer->sample_length : 0;
                        if (sampleLen > 0) {
                            uint32_t mouseSampleIdx = screenToSampleIndex(mousePos.x, wavePos, waveW, sampleLen);

                            // Apply zoom
                            fZoomLevel *= powf(ZOOM_SPEED, fZoomWheelAccum);
                            fZoomLevel = ImClamp(fZoomLevel, MIN_ZOOM, MAX_ZOOM);
                            fZoomWheelAccum -= (int)fZoomWheelAccum;

                            // Adjust pan to keep mouse position on same sample
                            uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
                            uint32_t desiredStartSample = mouseSampleIdx - (uint32_t)(mouseRelX * visibleSamples);
                            fPanOffset = (float)desiredStartSample / (float)sampleLen;

                            // Clamp pan
                            uint32_t maxStartSample = sampleLen - visibleSamples;
                            fPanOffset = ImClamp(fPanOffset, 0.0f,
                                                 (float)maxStartSample / (float)sampleLen);
                        }
                    }
                }

                // === PAN when zoomed (middle-mouse or Shift+left drag) ===
                if (fZoomLevel > 1.0f && isWaveformActive) {
                    uint32_t sampleLen = fUI->fSlicer ? fUI->fSlicer->sample_length : 0;

                    bool isPanning = ImGui::IsMouseDragging(2, 0.0f) ||  // Middle mouse
                                     (ImGui::IsMouseDragging(0, 0.0f) && io.KeyShift &&
                                      fMarkerState != MARKER_DRAGGING);

                    if (isPanning && sampleLen > 0) {
                        float dragDelta = io.MouseDelta.x;
                        uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
                        float sampleDelta = -(dragDelta / waveW) * visibleSamples;

                        uint32_t currentStartSample = (uint32_t)(fPanOffset * sampleLen);
                        int newStartSample = (int)currentStartSample + (int)sampleDelta;

                        // Clamp to valid range
                        if (newStartSample < 0) newStartSample = 0;
                        if (newStartSample > (int)(sampleLen - visibleSamples)) {
                            newStartSample = sampleLen - visibleSamples;
                        }

                        fPanOffset = (float)newStartSample / (float)sampleLen;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    }
                }

                // === DOUBLE-CLICK to Reset Zoom ===
                // (but not in play button area to avoid accidental resets)
                float playBarY = wavePos.y + waveH - 18;
                bool mouseInPlayArea = io.MousePos.y >= playBarY && io.MousePos.y <= wavePos.y + waveH;

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) &&
                    fHoveredMarkerIndex < 0 && !mouseInPlayArea) {
                    fZoomLevel = 1.0f;
                    fPanOffset = 0.0f;
                    fZoomWheelAccum = 0.0f;
                }

                bool hasSample = fUI->fSamplePath.isNotEmpty() && fUI->fSlicer && rgslicer_has_sample(fUI->fSlicer);

                if (hasSample) {
                    // Draw waveform
                    uint32_t sampleLen = fUI->fSlicer->sample_length;
                    int16_t* sampleData = fUI->fSlicer->sample_data;

                    if (sampleData && sampleLen > 0) {
                        const float centerY = wavePos.y + waveH * 0.5f;
                        const float amp = waveH * 0.4f;

                        // Draw waveform (with zoom/pan support)
                        // Calculate visible sample range
                        uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
                        uint32_t startSample = (uint32_t)(fPanOffset * sampleLen);

                        // Clamp to valid range
                        if (startSample + visibleSamples > sampleLen) {
                            startSample = sampleLen - visibleSamples;
                        }
                        uint32_t endSample = startSample + visibleSamples;

                        // Draw waveform for visible range
                        for (int x = 0; x < (int)waveW - 1; x++) {
                            uint32_t idx1 = startSample + (uint32_t)(((float)x / waveW) * visibleSamples);
                            uint32_t idx2 = startSample + (uint32_t)(((float)(x + 1) / waveW) * visibleSamples);

                            if (idx1 >= sampleLen) idx1 = sampleLen - 1;
                            if (idx2 >= sampleLen) idx2 = sampleLen - 1;

                            float y1 = centerY - (sampleData[idx1] / 32768.0f) * amp;
                            float y2 = centerY - (sampleData[idx2] / 32768.0f) * amp;

                            draw->AddLine(ImVec2(wavePos.x + x, y1), ImVec2(wavePos.x + x + 1, y2),
                                IM_COL32(100, 200, 100, 255), 1.0f);
                        }

                        // Show zoom level when zoomed
                        if (fZoomLevel > 1.0f) {
                            char zoomText[32];
                            snprintf(zoomText, sizeof(zoomText), "%.1fx", fZoomLevel);
                            ImVec2 textSize = ImGui::CalcTextSize(zoomText);
                            ImVec2 textPos(wavePos.x + waveW - textSize.x - 5, wavePos.y + waveH - textSize.y - 5);

                            draw->AddRectFilled(ImVec2(textPos.x - 3, textPos.y - 2),
                                               ImVec2(textPos.x + textSize.x + 3, textPos.y + textSize.y + 2),
                                               IM_COL32(0, 0, 0, 180));
                            draw->AddText(textPos, IM_COL32(255, 255, 255, 255), zoomText);
                        }

                        // === MARKER INTERACTION STATE MACHINE ===
                        ImVec2 mousePos = ImGui::GetMousePos();

                        // Define play button area at bottom of waveform (18px from bottom)
                        float playBarY = wavePos.y + waveH - 18;
                        bool isInPlayButtonArea = mousePos.y >= playBarY && mousePos.y <= wavePos.y + waveH;

                        switch (fMarkerState) {
                            case MARKER_IDLE: {
                                // Check for marker hover (but not in play button area)
                                if (isWaveformHovered && !isWaveformActive && !isInPlayButtonArea) {
                                    int hoveredMarker = findMarkerAtPosition(mousePos.x, wavePos, waveW, sampleLen);
                                    fHoveredMarkerIndex = hoveredMarker;

                                    if (hoveredMarker >= 0) {
                                        fMarkerState = MARKER_HOVERING;
                                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                                    }
                                }

                                // Left-click to add new marker (when not hovering over existing marker and not in play area)
                                if (leftClicked && fHoveredMarkerIndex < 0 && !isInPlayButtonArea) {
                                    uint32_t newOffset = screenToSampleIndex(mousePos.x, wavePos, waveW, sampleLen);
                                    int newSliceIdx = rgslicer_add_slice(fUI->fSlicer, newOffset);
                                    if (newSliceIdx >= 0) {
                                        fUI->fNumSlices = rgslicer_get_num_slices(fUI->fSlicer);
                                    }
                                }

                                // Right-click for file loading (when not over marker and not in play area)
                                if (rightClicked && fHoveredMarkerIndex < 0 && !isInPlayButtonArea) {
                                    fUI->requestStateFile("samplePath");
                                }
                                break;
                            }

                            case MARKER_HOVERING: {
                                // Update hover state
                                int hoveredMarker = findMarkerAtPosition(mousePos.x, wavePos, waveW, sampleLen);
                                fHoveredMarkerIndex = hoveredMarker;

                                if (hoveredMarker < 0) {
                                    fMarkerState = MARKER_IDLE;
                                } else {
                                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                                    // Start drag on left-click
                                    if (leftClicked) {
                                        fMarkerState = MARKER_DRAGGING;
                                        fDraggedMarkerIndex = hoveredMarker;
                                        fDragStartMouseX = mousePos.x;
                                        fDragStartOffset = rgslicer_get_slice_offset(fUI->fSlicer, hoveredMarker);
                                    }

                                    // Delete on right-click
                                    if (rightClicked) {
                                        // Clear playing slice if we're deleting it
                                        int playingSlice = (int)fUI->fParameters[PARAM_PLAYING_SLICE];
                                        if (playingSlice == hoveredMarker) {
                                            fUI->fParameters[PARAM_PLAYING_SLICE] = -1;
                                        } else if (playingSlice > hoveredMarker) {
                                            // Shift down playing slice index if it's after the deleted one
                                            fUI->fParameters[PARAM_PLAYING_SLICE] = playingSlice - 1;
                                        }

                                        rgslicer_remove_slice(fUI->fSlicer, hoveredMarker);
                                        fUI->fNumSlices = rgslicer_get_num_slices(fUI->fSlicer);
                                        fHoveredMarkerIndex = -1;
                                        fMarkerState = MARKER_IDLE;
                                    }
                                }
                                break;
                            }

                            case MARKER_DRAGGING: {
                                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                                // Update marker position while dragging
                                if (ImGui::IsMouseDragging(0, 0.0f)) {
                                    uint32_t newOffset = screenToSampleIndex(mousePos.x, wavePos, waveW, sampleLen);
                                    newOffset = ImClamp(newOffset, 0u, sampleLen - 1);
                                    rgslicer_move_slice(fUI->fSlicer, fDraggedMarkerIndex, newOffset);
                                }

                                // End drag on mouse release
                                if (!ImGui::IsMouseDown(0)) {
                                    fMarkerState = MARKER_IDLE;
                                    fDraggedMarkerIndex = -1;
                                    fHoveredMarkerIndex = -1;
                                }
                                break;
                            }
                        }

                        // Draw slice markers and active slice overlay
                        uint8_t numSlices = rgslicer_get_num_slices(fUI->fSlicer);
                        int playingSlice = (int)fUI->fParameters[PARAM_PLAYING_SLICE];

                        // If playing full sample (C#2, note 37), highlight entire waveform
                        if (playingSlice == -2) {
                            draw->AddRectFilled(
                                ImVec2(wavePos.x, wavePos.y),
                                ImVec2(wavePos.x + waveW, wavePos.y + waveH),
                                IM_COL32(255, 255, 0, 80)  // Yellow with 80/255 alpha
                            );
                        }

                        // Draw slice markers (only visible ones with state-based highlighting)
                        for (int i = 0; i < numSlices; i++) {
                            uint32_t offset = rgslicer_get_slice_offset(fUI->fSlicer, i);
                            float xPos = sampleIndexToScreenX(offset, wavePos, waveW, sampleLen);

                            // Skip if not visible
                            if (xPos < 0.0f) continue;

                            // Visual state
                            ImU32 markerColor;
                            float markerThickness;

                            if (i == fDraggedMarkerIndex) {
                                markerColor = IM_COL32(255, 255, 0, 255);   // Bright yellow
                                markerThickness = 3.0f;
                            } else if (i == fHoveredMarkerIndex) {
                                markerColor = IM_COL32(255, 255, 150, 255); // Light yellow
                                markerThickness = 2.5f;
                            } else {
                                markerColor = IM_COL32(255, 255, 0, 200);   // Normal yellow
                                markerThickness = 2.0f;
                            }

                            // Draw marker line
                            draw->AddLine(ImVec2(xPos, wavePos.y), ImVec2(xPos, wavePos.y + waveH),
                                          markerColor, markerThickness);

                            // Active slice overlay
                            if (i == playingSlice) {
                                uint32_t length = rgslicer_get_slice_length(fUI->fSlicer, i);
                                float xEnd = sampleIndexToScreenX(offset + length, wavePos, waveW, sampleLen);

                                if (xEnd >= wavePos.x) {
                                    float overlayStart = ImMax(xPos, wavePos.x);
                                    float overlayEnd = ImMin(xEnd, wavePos.x + waveW);

                                    draw->AddRectFilled(
                                        ImVec2(overlayStart, wavePos.y),
                                        ImVec2(overlayEnd, wavePos.y + waveH),
                                        IM_COL32(255, 255, 0, 80)
                                    );
                                }
                            }

                            // Slice number
                            char sliceNum[8];
                            snprintf(sliceNum, sizeof(sliceNum), "%d", i);
                            draw->AddText(ImVec2(xPos + 2, wavePos.y + 2),
                                          IM_COL32(255, 255, 0, 255), sliceNum);
                        }

                        // Draw playback position marker (red vertical line)
                        float playbackPos = fUI->fParameters[PARAM_PLAYBACK_POS];
                        if (playbackPos > 0.0f && playbackPos <= 1.0f) {
                            // Convert normalized position to sample index
                            uint32_t playSampleIdx = (uint32_t)(playbackPos * sampleLen);

                            // Auto-pan to follow playhead when zoomed in (only if playhead goes outside visible range)
                            if (fZoomLevel > 1.0f) {
                                uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
                                uint32_t startSample = (uint32_t)(fPanOffset * sampleLen);
                                uint32_t endSample = startSample + visibleSamples;

                                // Only pan if playhead is outside the visible range
                                if (playSampleIdx < startSample || playSampleIdx > endSample) {
                                    // Center playhead in view
                                    uint32_t newStartSample = playSampleIdx > visibleSamples / 2
                                        ? playSampleIdx - visibleSamples / 2
                                        : 0;

                                    // Clamp to valid range
                                    if (newStartSample + visibleSamples > sampleLen) {
                                        newStartSample = sampleLen - visibleSamples;
                                    }

                                    fPanOffset = (float)newStartSample / (float)sampleLen;
                                }
                            }

                            // Convert to screen coordinates (zoom-aware)
                            float xPos = sampleIndexToScreenX(playSampleIdx, wavePos, waveW, sampleLen);

                            // Only draw if visible
                            if (xPos >= 0.0f) {
                                draw->AddLine(ImVec2(xPos, wavePos.y), ImVec2(xPos, wavePos.y + waveH),
                                    IM_COL32(255, 0, 0, 255), 3.0f);  // Red playback marker
                            }
                        }

                        // === PREVIEW PLAY MARKERS (inside waveform at bottom) ===
                        if (numSlices > 0) {
                            float playBarY = wavePos.y + waveH - 18;
                            float playBarH = 16.0f;

                            for (int i = 0; i < numSlices; i++) {
                                uint32_t offset = rgslicer_get_slice_offset(fUI->fSlicer, i);
                                uint32_t length = rgslicer_get_slice_length(fUI->fSlicer, i);

                                float xStart = sampleIndexToScreenX(offset, wavePos, waveW, sampleLen);
                                float xEnd = sampleIndexToScreenX(offset + length, wavePos, waveW, sampleLen);

                                // Skip if completely outside visible range
                                if (xEnd < wavePos.x || xStart > wavePos.x + waveW) continue;

                                // Clamp to visible area
                                float buttonX = ImMax(xStart, wavePos.x);
                                float buttonW = ImMin(xEnd, wavePos.x + waveW) - buttonX;

                                // Ensure minimum width for clickability
                                if (buttonW < 16.0f && xStart >= wavePos.x) {
                                    buttonW = ImMin(16.0f, wavePos.x + waveW - buttonX);
                                }

                                ImVec2 buttonMin(buttonX, playBarY);
                                ImVec2 buttonMax(buttonX + buttonW, playBarY + playBarH);

                                // Check if mouse is hovering this button
                                ImVec2 mousePos = ImGui::GetMousePos();
                                bool isHovered = mousePos.x >= buttonMin.x && mousePos.x <= buttonMax.x &&
                                                mousePos.y >= buttonMin.y && mousePos.y <= buttonMax.y;

                                // Button color based on state
                                ImU32 buttonColor;
                                if (i == playingSlice) {
                                    buttonColor = IM_COL32(255, 200, 0, 200);  // Yellow when playing
                                } else if (isHovered) {
                                    buttonColor = IM_COL32(80, 80, 80, 180);  // Gray when hovered
                                } else {
                                    buttonColor = IM_COL32(40, 40, 40, 150);  // Dark gray default
                                }

                                // Draw button
                                draw->AddRectFilled(buttonMin, buttonMax, buttonColor);
                                draw->AddRect(buttonMin, buttonMax, IM_COL32(100, 100, 100, 200), 0.0f, 0, 1.0f);

                                // Draw play triangle (pointing right)
                                if (buttonW >= 8.0f) {
                                    float centerX = buttonX + buttonW * 0.5f;
                                    float centerY = playBarY + playBarH * 0.5f;
                                    float triSize = ImMin(6.0f, buttonW * 0.4f);

                                    ImVec2 p1(centerX - triSize * 0.5f, centerY - triSize * 0.6f);  // Top left
                                    ImVec2 p2(centerX - triSize * 0.5f, centerY + triSize * 0.6f);  // Bottom left
                                    ImVec2 p3(centerX + triSize * 0.5f, centerY);                   // Right point

                                    draw->AddTriangleFilled(p1, p2, p3, IM_COL32(220, 220, 220, 255));
                                }

                                // Handle click to trigger slice playback
                                if (isHovered && ImGui::IsMouseClicked(0)) {
                                    // Find the MIDI note mapped to this slice
                                    uint8_t triggeredNote = 0xFF;
                                    for (uint8_t note = 0; note < 128; note++) {
                                        if (fUI->fSlicer->note_map[note] == i) {
                                            triggeredNote = note;
                                            break;
                                        }
                                    }

                                    // Trigger with toggle bit (ensures unique value every click)
                                    if (triggeredNote != 0xFF) {
                                        fTriggerToggle = !fTriggerToggle;
                                        float triggerValue = (float)(triggeredNote + (fTriggerToggle ? 128 : 0));
                                        fUI->setParameterValue(PARAM_TRIGGER_NOTE, triggerValue);
                                    }
                                }
                            }
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

                // ===== IMPORT BUTTON =====
                ImGui::SetCursorPosX(pad);

                if (ImGui::Button("Import WAV/SFZ", ImVec2(140, 25))) {
                    fUI->requestStateFile("samplePath");
                }

                // ===== EXPORT BUTTONS =====
                if (hasSample && fUI->fNumSlices > 0) {
                    ImGui::SameLine(0, pad);

                    if (ImGui::Button("Export WAV+CUE", ImVec2(140, 25))) {
                        // Request file path for export (write mode)
                        fUI->requestStateFile("exportWavCue");
                    }

                    ImGui::SameLine(0, pad);

                    if (ImGui::Button("Export SFZ", ImVec2(140, 25))) {
                        fUI->requestStateFile("exportSfz");
                    }

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
                        fUI->fParameters[PARAM_MASTER_VOLUME] = vol * 100.0f;
                        fUI->setParameterValue(PARAM_MASTER_VOLUME, vol * 100.0f);
                    }
                    // Double-click to reset to 100%
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        fUI->fParameters[PARAM_MASTER_VOLUME] = 100.0f;
                        fUI->setParameterValue(PARAM_MASTER_VOLUME, 100.0f);
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
                        fUI->fParameters[PARAM_MASTER_PITCH] = pitch * 24.0f - 12.0f;
                        fUI->setParameterValue(PARAM_MASTER_PITCH, pitch * 24.0f - 12.0f);
                    }
                    // Double-click to reset to 0 semitones
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        fUI->fParameters[PARAM_MASTER_PITCH] = 0.0f;
                        fUI->setParameterValue(PARAM_MASTER_PITCH, 0.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Master Time
                float time = (fUI->fParameters[PARAM_MASTER_TIME] - 10.0f) / 790.0f;
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
                        fUI->fParameters[PARAM_MASTER_TIME] = time * 790.0f + 10.0f;
                        fUI->setParameterValue(PARAM_MASTER_TIME, time * 790.0f + 10.0f);
                    }
                    // Double-click to reset to 100%
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        fUI->fParameters[PARAM_MASTER_TIME] = 100.0f;
                        fUI->setParameterValue(PARAM_MASTER_TIME, 100.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // BPM (for random sequencer)
                float bpm = (fUI->fParameters[PARAM_BPM] - 20.0f) / 280.0f;
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("BPM").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("BPM");
                    ImGui::PopStyleColor();

                    if (ImGuiKnobs::Knob("##bpm", &bpm, 0.0f, 1.0f, 0.01f,
                                         "", ImGuiKnobVariant_Tick, knobSize,
                                         ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput, 10))
                    {
                        fUI->fParameters[PARAM_BPM] = bpm * 280.0f + 20.0f;
                        fUI->setParameterValue(PARAM_BPM, bpm * 280.0f + 20.0f);
                    }

                    // BPM value label
                    char bpmVal[8];
                    snprintf(bpmVal, sizeof(bpmVal), "%d", (int)fUI->fParameters[PARAM_BPM]);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    float bpmValW = ImGui::CalcTextSize(bpmVal).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - bpmValW) * 0.5f);
                    ImGui::Text("%s", bpmVal);
                    ImGui::PopStyleColor();
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // Note Division (for random sequencer)
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
                    float labelW = ImGui::CalcTextSize("NOTE DIV").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (knobSize - labelW) * 0.5f);
                    ImGui::Text("NOTE DIV");
                    ImGui::PopStyleColor();

                    // Dropdown for note division
                    const char* divisionLabels[] = { "1/4", "1/8", "1/16", "1/32" };
                    int currentDiv = 0;
                    float div = fUI->fParameters[PARAM_NOTE_DIVISION];
                    if (div <= 1.5f) currentDiv = 0;       // Quarter
                    else if (div <= 3.0f) currentDiv = 1;  // 8th
                    else if (div <= 6.0f) currentDiv = 2;  // 16th
                    else currentDiv = 3;                   // 32nd

                    ImGui::PushItemWidth(knobSize);
                    if (ImGui::Combo("##notediv", &currentDiv, divisionLabels, 4)) {
                        float divValues[] = { 1.0f, 2.0f, 4.0f, 8.0f };
                        fUI->fParameters[PARAM_NOTE_DIVISION] = divValues[currentDiv];
                        fUI->setParameterValue(PARAM_NOTE_DIVISION, divValues[currentDiv]);
                    }
                    ImGui::PopItemWidth();
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
                        fUI->fParameters[PARAM_SLICE0_PITCH] = s0pitch * 24.0f - 12.0f;
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
                        fUI->fParameters[PARAM_SLICE0_TIME] = s0time * 150.0f + 50.0f;
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
                        fUI->fParameters[PARAM_SLICE0_VOLUME] = s0vol * 200.0f;
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
                        fUI->fParameters[PARAM_SLICE0_PAN] = s0pan * 200.0f - 100.0f;
                        fUI->setParameterValue(PARAM_SLICE0_PAN, s0pan * 200.0f - 100.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // S0 Loop toggle
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 LOOP");
                    bool loop = fUI->fParameters[PARAM_SLICE0_LOOP] > 0.5f;
                    if (ImGui::Checkbox("##s0loop", &loop))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_LOOP, loop ? 1.0f : 0.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, knobSpacing);

                // S0 One-Shot toggle
                ImGui::BeginGroup();
                {
                    ImGui::Text("S0 ONE-SHOT");
                    bool oneshot = fUI->fParameters[PARAM_SLICE0_ONE_SHOT] > 0.5f;
                    if (ImGui::Checkbox("##s0oneshot", &oneshot))
                    {
                        fUI->setParameterValue(PARAM_SLICE0_ONE_SHOT, oneshot ? 1.0f : 0.0f);
                    }
                }
                ImGui::EndGroup();

                ImGui::End();
            }
        }

    private:
        // Coordinate conversion helpers
        uint32_t screenToSampleIndex(float screenX, const ImVec2& wavePos,
                                     float waveW, uint32_t sampleLen) const;
        float sampleIndexToScreenX(uint32_t sampleIdx, const ImVec2& wavePos,
                                   float waveW, uint32_t sampleLen) const;
        int findMarkerAtPosition(float screenX, const ImVec2& wavePos,
                                float waveW, uint32_t sampleLen) const;

        RGSlicerUI* const fUI;

        // Waveform zoom and pan state
        float fZoomLevel;              // 1.0 = 100%, up to 20.0 = max zoom
        float fPanOffset;              // Sample offset for left edge (0.0-1.0 normalized)
        float fZoomWheelAccum;         // Smooth wheel zoom accumulator

        // Marker interaction state
        enum MarkerDragState {
            MARKER_IDLE,
            MARKER_HOVERING,
            MARKER_DRAGGING
        };
        MarkerDragState fMarkerState;
        int fHoveredMarkerIndex;       // -1 = none
        int fDraggedMarkerIndex;       // -1 = none
        float fDragStartMouseX;
        uint32_t fDragStartOffset;

        // Trigger toggle for repeatable clicks (alternates 0/1)
        bool fTriggerToggle;

        // Constants
        static constexpr float MARKER_HIT_TOLERANCE = 5.0f;
        static constexpr float MIN_ZOOM = 1.0f;
        static constexpr float MAX_ZOOM = 20.0f;
        static constexpr float ZOOM_SPEED = 1.1f;
    };

    RGSlicerImGuiWidget* fImGuiWidget;
};

// ============================================================================
// RGSlicerImGuiWidget Helper Method Implementations
// ============================================================================

uint32_t RGSlicerUI::RGSlicerImGuiWidget::screenToSampleIndex(
    float screenX, const ImVec2& wavePos, float waveW, uint32_t sampleLen) const
{
    // Convert screen X to relative position (0.0 to 1.0) within waveform area
    float relX = (screenX - wavePos.x) / waveW;
    relX = ImClamp(relX, 0.0f, 1.0f);

    // Calculate visible sample range
    uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
    uint32_t startSample = (uint32_t)(fPanOffset * sampleLen);

    // Clamp start to ensure we don't go past the end
    if (startSample + visibleSamples > sampleLen) {
        startSample = sampleLen - visibleSamples;
    }

    // Map relative position to sample index
    uint32_t sampleIdx = startSample + (uint32_t)(relX * visibleSamples);
    return ImClamp(sampleIdx, 0u, sampleLen - 1);
}

float RGSlicerUI::RGSlicerImGuiWidget::sampleIndexToScreenX(
    uint32_t sampleIdx, const ImVec2& wavePos, float waveW, uint32_t sampleLen) const
{
    // Calculate visible sample range
    uint32_t visibleSamples = (uint32_t)(sampleLen / fZoomLevel);
    uint32_t startSample = (uint32_t)(fPanOffset * sampleLen);

    // Clamp start to ensure we don't go past the end
    if (startSample + visibleSamples > sampleLen) {
        startSample = sampleLen - visibleSamples;
    }

    uint32_t endSample = startSample + visibleSamples;

    // Check if sample is visible
    if (sampleIdx < startSample || sampleIdx > endSample) {
        return -1.0f;  // Not visible
    }

    // Calculate relative position within visible range
    float relPos = (float)(sampleIdx - startSample) / (float)visibleSamples;
    return wavePos.x + relPos * waveW;
}

int RGSlicerUI::RGSlicerImGuiWidget::findMarkerAtPosition(
    float screenX, const ImVec2& wavePos, float waveW, uint32_t sampleLen) const
{
    if (!fUI->fSlicer) return -1;

    uint8_t numSlices = rgslicer_get_num_slices(fUI->fSlicer);

    // Check each marker (iterate backwards to prioritize front markers)
    for (int i = numSlices - 1; i >= 0; i--) {
        uint32_t offset = rgslicer_get_slice_offset(fUI->fSlicer, i);
        float markerX = sampleIndexToScreenX(offset, wavePos, waveW, sampleLen);

        // Check if marker is visible and within tolerance
        if (markerX >= 0.0f && fabs(screenX - markerX) <= MARKER_HIT_TOLERANCE) {
            return i;
        }
    }

    return -1;
}

UI* createUI()
{
    return new RGSlicerUI();
}

END_NAMESPACE_DISTRHO
