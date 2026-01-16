#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "DistrhoPluginInfo.h"
#include "../regroove_ui_helpers.hpp"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGDeckPlayerUI : public UI
{
public:
    RGDeckPlayerUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
        , fLastOrder(0)
        , fLastRow(0)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.0f;
        }

        fParameters[kParameterBPM] = 1.0f;  // 100% tempo
        fParameters[kParameterLoopEnd] = 127.0f;
        fParameters[kParameterCh1Volume] = 1.0f;
        fParameters[kParameterCh2Volume] = 1.0f;
        fParameters[kParameterCh3Volume] = 1.0f;
        fParameters[kParameterCh4Volume] = 1.0f;
        fParameters[kParameterCh1Pan] = -0.5f;
        fParameters[kParameterCh2Pan] = 0.5f;
        fParameters[kParameterCh3Pan] = 0.5f;
        fParameters[kParameterCh4Pan] = -0.5f;

        fImGuiWidget = new RGDeckPlayerImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGDeckPlayerUI() override { delete fImGuiWidget; }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;
            fImGuiWidget->repaint();
        }
    }

    void stateChanged(const char* key, const char* value) override
    {
        if (std::strcmp(key, "file") == 0) {
            fFilePath = value ? value : "";
            fImGuiWidget->repaint();
        }
    }

    void uiIdle() override
    {
        // Check if output parameters (Order/Row) changed - repaint only if needed
        uint8_t currentOrder = (uint8_t)fParameters[kParameterCurrentOrder];
        uint16_t currentRow = (uint16_t)fParameters[kParameterCurrentRow];

        if (currentOrder != fLastOrder || currentRow != fLastRow) {
            fLastOrder = currentOrder;
            fLastRow = currentRow;
            fImGuiWidget->repaint();
        }
    }

    void uiReshape(uint width, uint height) override {
        UI::uiReshape(width, height);
        fImGuiWidget->setSize(width, height);
    }

private:
    friend class RGDeckPlayerImGuiWidget;
    float fParameters[kParameterCount];
    String fFilePath;
    uint8_t fLastOrder;
    uint16_t fLastRow;

    class RGDeckPlayerImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGDeckPlayerImGuiWidget(RGDeckPlayerUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            ImGuiStyle& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_WindowBg] = RegrooveColors::BG;
            style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
            style.FrameRounding = 6.0f;
            style.WindowPadding = ImVec2(0, 0);  // No window padding

            if (ImGui::Begin(RGDECKPLAYER_WINDOW_TITLE, nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
            {
                const float pad = 4.0f;  // Tighter spacing
                const float padSize = 58.0f;  // Smaller pads to fit better

                ImDrawList* draw = ImGui::GetWindowDrawList();

                // Red header - AT THE TOP
                draw->AddRectFilled(ImVec2(0, 0), ImVec2(width, 26),
                    IM_COL32(RegrooveColors::RED_R, RegrooveColors::RED_G, RegrooveColors::RED_B, 255));

                ImGui::SetCursorPosY(6);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                float tw = ImGui::CalcTextSize("Tracker Deck").x;
                ImGui::SetCursorPosX((width - tw) * 0.5f);
                ImGui::Text("Tracker Deck");
                ImGui::PopStyleColor();
                ImGui::SetCursorPosY(26 + pad);  // Margin below header

                // Position panel (full width)
                ImGui::SetCursorPosX(pad);
                ImVec2 panelP = ImGui::GetCursorScreenPos();
                const float panelH = 85.0f;
                const float panelW = width - 2*pad;
                draw->AddRectFilled(panelP, ImVec2(panelP.x + panelW, panelP.y + panelH),
                    IM_COL32(0, 0, 0, 255), 4.0f);
                draw->AddRect(panelP, ImVec2(panelP.x + panelW, panelP.y + panelH),
                    IM_COL32(RegrooveColors::RED_R, RegrooveColors::RED_G, RegrooveColors::RED_B, 255),
                    4.0f, 0, 2.0f);

                // Invisible button for right-click file loading
                ImGui::SetCursorScreenPos(panelP);
                ImGui::InvisibleButton("##panel", ImVec2(panelW, panelH));
                if (ImGui::IsItemClicked(1)) {  // Right mouse button
                    fUI->requestStateFile("file");
                }

                bool hasFile = fUI->fFilePath.isNotEmpty();

                if (hasFile) {
                    // Show position info - use absolute positioning
                    ImGui::SetCursorScreenPos(ImVec2(panelP.x + 6, panelP.y + 6));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

                    // Get current position from output parameters
                    uint8_t order = (uint8_t)fUI->fParameters[kParameterCurrentOrder];
                    uint16_t row = (uint16_t)fUI->fParameters[kParameterCurrentRow];

                    ImGui::Text("Order: %02d", order);
                    ImGui::SetCursorScreenPos(ImVec2(panelP.x + 6, panelP.y + 22));
                    ImGui::Text("Pattern: --");
                    ImGui::SetCursorScreenPos(ImVec2(panelP.x + 6, panelP.y + 38));
                    ImGui::Text("Row: %02d", row);
                    ImGui::PopStyleColor();

                    // Filename INSIDE panel
                    const char* p = fUI->fFilePath.buffer();
                    const char* slash = strrchr(p, '/');
                    if (!slash) slash = strrchr(p, '\\');
                    const char* fn = slash ? (slash + 1) : p;

                    ImGui::SetCursorScreenPos(ImVec2(panelP.x + 6, panelP.y + 62));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::Text("%s", fn);
                    ImGui::PopStyleColor();
                } else {
                    // No file loaded - show message centered
                    ImGui::SetCursorScreenPos(ImVec2(panelP.x + (panelW - ImGui::CalcTextSize("No file loaded").x) * 0.5f, panelP.y + 38));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                    ImGui::Text("No file loaded");
                    ImGui::PopStyleColor();
                }

                ImGui::SetCursorPosY(panelP.y + panelH + 8);

                // Calculate layout: channel buttons use full width, pads have tempo slider on right
                const float sliderW = 38.0f;  // Tempo slider width (wider to balance smaller pads)
                const float contentW = panelW - sliderW - pad;  // Content width for PADS only

                // Channel buttons - use FULL panel width (no tempo slider space)
                const uint32_t muteP[] = {kParameterCh1Mute, kParameterCh2Mute, kParameterCh3Mute, kParameterCh4Mute};
                const float chSize = 38.0f;
                const float chInset = 12.0f;  // Inset from edge
                const float chTotal = 4 * chSize;
                const float chGap = (panelW - chTotal - 2 * chInset) / 3.0f;  // Space between buttons

                ImGui::SetCursorPosX(pad + chInset);
                for (int i = 0; i < 4; i++) {
                    if (i > 0) ImGui::SameLine(0, chGap);

                    bool muted = fUI->fParameters[muteP[i]] > 0.5f;
                    ImGui::PushID(i);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, muted ?
                        ImVec4(0.8f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 0.85f, 0.0f, 1.0f));  // Red when muted
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, muted ?
                        ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.0f, 0.95f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, muted ?
                        ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.1f, 1.0f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, muted ?
                        ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f));  // White text on red

                    char lbl[8];
                    snprintf(lbl, sizeof(lbl), "CH%d", i + 1);
                    if (ImGui::Button(lbl, ImVec2(chSize, chSize))) {
                        float newValue = muted ? 0.0f : 1.0f;
                        fUI->fParameters[muteP[i]] = newValue;  // Update local state immediately
                        fUI->setParameterValue(muteP[i], newValue);
                    }

                    ImGui::PopStyleColor(4);
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

                // LOOP and PLAY - SQUARE pads with consistent spacing
                float padsX = pad + 8.0f;  // Consistent left margin
                float loopPlayY = ImGui::GetCursorScreenPos().y;  // Save Y position for tempo slider
                ImGui::SetCursorPosX(padsX);

                bool loopOn = fUI->fParameters[kParameterLoopPattern] > 0.5f;
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                // LOOP: Grey when inactive, Yellow when active
                ImGui::PushStyleColor(ImGuiCol_Button, loopOn ?
                    ImVec4(1.0f, 0.88f, 0.0f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, loopOn ?
                    ImVec4(1.0f, 0.92f, 0.1f, 1.0f) : ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, loopOn ?
                    ImVec4(1.0f, 0.95f, 0.2f, 1.0f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                if (ImGui::Button("LOOP", ImVec2(padSize, padSize))) {
                    float newValue = loopOn ? 0.0f : 1.0f;
                    fUI->fParameters[kParameterLoopPattern] = newValue;  // Update local state immediately
                    fUI->setParameterValue(kParameterLoopPattern, newValue);
                }
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar();

                ImGui::SameLine(0, 8.0f);

                // PLAY: Grey when no file, Red when stopped with file, Green when playing
                bool playOn = fUI->fParameters[kParameterPlay] > 0.5f;
                bool hasFileForPlay = fUI->fFilePath.isNotEmpty();
                ImVec4 playColor;
                if (!hasFileForPlay) {
                    playColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);  // Grey - no file
                } else if (playOn) {
                    playColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green - playing
                } else {
                    playColor = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);  // Red - stopped with file
                }

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, playColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, playOn ?
                    ImVec4(0.1f, 1.0f, 0.1f, 1.0f) : (hasFileForPlay ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.35f, 0.35f, 0.35f, 1.0f)));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, playOn ?
                    ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (hasFileForPlay ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f)));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                if (ImGui::Button("PLAY", ImVec2(padSize, padSize))) {
                    float newValue = playOn ? 0.0f : 1.0f;
                    fUI->fParameters[kParameterPlay] = newValue;  // Update local state immediately
                    fUI->setParameterValue(kParameterPlay, newValue);
                }
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar();

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);  // Tighter vertical spacing

                // PTN- PTN+ - SQUARE 70x70 (use invisible button + text overlay)
                ImGui::SetCursorPosX(padsX);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.38f, 0.38f, 1.0f));

                // PTN- button
                ImVec2 ptn1Pos = ImGui::GetCursorScreenPos();
                if (ImGui::Button("##ptn_minus", ImVec2(padSize, padSize))) {
                    fUI->setParameterValue(kParameterPrevPattern, 1.0f);
                }
                // Draw centered text
                const char* ptn1Text = "PTN-";
                float ptn1TextW = ImGui::CalcTextSize(ptn1Text).x;
                draw->AddText(ImVec2(ptn1Pos.x + (padSize - ptn1TextW) * 0.5f, ptn1Pos.y + padSize * 0.4f),
                    IM_COL32(200, 200, 200, 255), ptn1Text);

                ImGui::SameLine(0, 8.0f);

                // PTN+ button
                ImVec2 ptn2Pos = ImGui::GetCursorScreenPos();
                if (ImGui::Button("##ptn_plus", ImVec2(padSize, padSize))) {
                    fUI->setParameterValue(kParameterNextPattern, 1.0f);
                }
                // Draw centered text
                const char* ptn2Text = "PTN+";
                float ptn2TextW = ImGui::CalcTextSize(ptn2Text).x;
                draw->AddText(ImVec2(ptn2Pos.x + (padSize - ptn2TextW) * 0.5f, ptn2Pos.y + padSize * 0.4f),
                    IM_COL32(200, 200, 200, 255), ptn2Text);

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);  // Tighter vertical spacing

                // MUTE PFL - SQUARE pads
                ImGui::SetCursorPosX(padsX);

                // MUTE button - master mute for priming (doesn't change channel states)
                bool masterMuted = fUI->fParameters[kParameterMasterMute] > 0.5f;
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, masterMuted ?
                    ImVec4(0.8f, 0.0f, 0.0f, 1.0f) : ImVec4(0.18f, 0.18f, 0.18f, 1.0f));  // Red when active
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, masterMuted ?
                    ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, masterMuted ?
                    ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.38f, 0.38f, 0.38f, 1.0f));

                ImVec2 mutePos = ImGui::GetCursorScreenPos();
                if (ImGui::Button("##mute", ImVec2(padSize, padSize))) {
                    float newValue = masterMuted ? 0.0f : 1.0f;
                    fUI->fParameters[kParameterMasterMute] = newValue;
                    fUI->setParameterValue(kParameterMasterMute, newValue);
                }
                // Draw centered text
                const char* muteText = "MUTE";
                float muteTextW = ImGui::CalcTextSize(muteText).x;
                ImVec4 muteTextColor = masterMuted ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                draw->AddText(ImVec2(mutePos.x + (padSize - muteTextW) * 0.5f, mutePos.y + padSize * 0.4f),
                    IM_COL32((int)(muteTextColor.x*255), (int)(muteTextColor.y*255), (int)(muteTextColor.z*255), 255), muteText);

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                ImGui::SameLine(0, 8.0f);

                // PFL button (placeholder)
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.38f, 0.38f, 1.0f));

                ImVec2 pflPos = ImGui::GetCursorScreenPos();
                if (ImGui::Button("##pfl", ImVec2(padSize, padSize))) {
                    // PFL placeholder
                }
                // Draw centered text
                const char* pflText = "PFL";
                float pflTextW = ImGui::CalcTextSize(pflText).x;
                draw->AddText(ImVec2(pflPos.x + (padSize - pflTextW) * 0.5f, pflPos.y + padSize * 0.4f),
                    IM_COL32(200, 200, 200, 255), pflText);

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                // Tempo slider - RIGHT side, starts at LOOP/PLAY level
                float tempoMult = fUI->fParameters[kParameterBPM];  // 0.9 - 1.1
                float tempoNorm = (tempoMult - 0.9f) / (1.1f - 0.9f);
                const float sliderX = pad + contentW - 8.0f;  // Move more towards left
                const float sliderH = 3 * padSize + 2 * 6.0f;  // Height to cover 3 rows of pads + 2 gaps

                // Slider with rounded corners
                ImGui::SetCursorScreenPos(ImVec2(sliderX, loopPlayY));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, RegrooveColors::RED);
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.9f, 0.1f, 0.2f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);  // Rounded corners
                ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);  // Rounded grab

                if (ImGui::VSliderFloat("##tempo", ImVec2(sliderW, sliderH), &tempoNorm, 0.0f, 1.0f, "")) {
                    float newTempoMult = 0.9f + tempoNorm * (1.1f - 0.9f);
                    fUI->fParameters[kParameterBPM] = newTempoMult;
                    fUI->setParameterValue(kParameterBPM, newTempoMult);
                }

                ImGui::PopStyleVar(3);
                ImGui::PopStyleColor(3);

                ImGui::End();
            }
        }

    private:
        RGDeckPlayerUI* const fUI;
    };

    RGDeckPlayerImGuiWidget* fImGuiWidget;
};

UI* createUI()
{
    return new RGDeckPlayerUI();
}

END_NAMESPACE_DISTRHO
