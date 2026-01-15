#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

class RGModPlayerUI : public UI
{
public:
    RGModPlayerUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // Initialize parameters
        for (uint32_t i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.0f;
        }

        // Set defaults
        fParameters[kParameterSpeed] = 6.0f;
        fParameters[kParameterBPM] = 125.0f;
        fParameters[kParameterCh1Volume] = 1.0f;
        fParameters[kParameterCh2Volume] = 1.0f;
        fParameters[kParameterCh3Volume] = 1.0f;
        fParameters[kParameterCh4Volume] = 1.0f;
        fParameters[kParameterCh1Pan] = -0.5f;
        fParameters[kParameterCh2Pan] = 0.5f;
        fParameters[kParameterCh3Pan] = 0.5f;
        fParameters[kParameterCh4Pan] = -0.5f;

        fImGuiWidget = new RGModPlayerImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RGModPlayerUI() override { delete fImGuiWidget; }

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
            fModFilePath = value ? value : "";
            fImGuiWidget->repaint();
        }
    }

    void uiIdle() override { fImGuiWidget->repaint(); }
    void uiReshape(uint width, uint height) override {
        UI::uiReshape(width, height);
        fImGuiWidget->setSize(width, height);
    }

private:
    friend class RGModPlayerImGuiWidget;
    float fParameters[kParameterCount];
    String fModFilePath;

    class RGModPlayerImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RGModPlayerImGuiWidget(RGModPlayerUI* const ui) : ImGuiSubWidget(ui), fUI(ui) {}

    protected:
        void onImGuiDisplay() override
        {
            const float width = getWidth();
            const float height = getHeight();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));

            if (ImGui::Begin(RGMODPLAYER_WINDOW_TITLE, nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::Text("RGModPlayer - ProTracker Module Player");
                ImGui::Separator();

                // File section
                ImGui::Text("MOD File:");
                ImGui::SameLine();
                if (fUI->fModFilePath.isNotEmpty()) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", fUI->fModFilePath.buffer());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No file loaded");
                }

                if (ImGui::Button("Load MOD File...")) {
                    fUI->requestStateFile("file");
                }

                ImGui::Separator();

                // Playback controls
                ImGui::Text("Playback Controls");

                bool playing = fUI->fParameters[kParameterPlay] > 0.5f;
                if (ImGui::Checkbox("Play", &playing)) {
                    fUI->setParameterValue(kParameterPlay, playing ? 1.0f : 0.0f);
                }

                float loopStart = fUI->fParameters[kParameterLoopStart];
                if (ImGui::SliderFloat("Loop Start", &loopStart, 0.0f, 127.0f, "%.0f")) {
                    fUI->setParameterValue(kParameterLoopStart, loopStart);
                }

                float loopEnd = fUI->fParameters[kParameterLoopEnd];
                if (ImGui::SliderFloat("Loop End", &loopEnd, 0.0f, 127.0f, "%.0f")) {
                    fUI->setParameterValue(kParameterLoopEnd, loopEnd);
                }

                float speed = fUI->fParameters[kParameterSpeed];
                if (ImGui::SliderFloat("Speed", &speed, 1.0f, 31.0f, "%.0f")) {
                    fUI->setParameterValue(kParameterSpeed, speed);
                }

                float bpm = fUI->fParameters[kParameterBPM];
                if (ImGui::SliderFloat("BPM", &bpm, 32.0f, 255.0f, "%.0f")) {
                    fUI->setParameterValue(kParameterBPM, bpm);
                }

                ImGui::Separator();

                // Channel controls
                ImGui::Text("Channel Controls");

                const char* channelNames[] = {"Channel 1", "Channel 2", "Channel 3", "Channel 4"};
                const uint32_t muteParams[] = {kParameterCh1Mute, kParameterCh2Mute, kParameterCh3Mute, kParameterCh4Mute};
                const uint32_t volumeParams[] = {kParameterCh1Volume, kParameterCh2Volume, kParameterCh3Volume, kParameterCh4Volume};
                const uint32_t panParams[] = {kParameterCh1Pan, kParameterCh2Pan, kParameterCh3Pan, kParameterCh4Pan};

                for (int i = 0; i < 4; i++) {
                    ImGui::PushID(i);

                    ImGui::Text("%s", channelNames[i]);

                    bool muted = fUI->fParameters[muteParams[i]] > 0.5f;
                    if (ImGui::Checkbox("Mute", &muted)) {
                        fUI->setParameterValue(muteParams[i], muted ? 1.0f : 0.0f);
                    }

                    ImGui::SameLine();

                    float volume = fUI->fParameters[volumeParams[i]];
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f")) {
                        fUI->setParameterValue(volumeParams[i], volume);
                    }

                    ImGui::SameLine();

                    float pan = fUI->fParameters[panParams[i]];
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderFloat("Pan", &pan, -1.0f, 1.0f, "%.2f")) {
                        fUI->setParameterValue(panParams[i], pan);
                    }

                    ImGui::PopID();
                }

                ImGui::End();
            }
        }

    private:
        RGModPlayerUI* const fUI;
    };

    RGModPlayerImGuiWidget* fImGuiWidget;
};

UI* createUI()
{
    return new RGModPlayerUI();
}

END_NAMESPACE_DISTRHO
