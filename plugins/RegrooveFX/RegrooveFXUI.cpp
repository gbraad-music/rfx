/*
 * RegrooveFX Plugin UI - DPF Wrapper
 * Copyright (C) 2024
 *
 * This is just a thin wrapper around the framework-agnostic UI in regroove_effects_ui.h
 * The actual ImGui rendering is framework-independent and can be used in:
 * - DPF plugins (this file)
 * - SDL applications (../regroove, ../samplecrate)
 * - Any other ImGui integration
 */

#include "DistrhoUI.hpp"
#include "DearImGui.hpp"
#include "regroove_effects_ui.h"  // Framework-agnostic UI

START_NAMESPACE_DISTRHO
USE_NAMESPACE_DGL

// -----------------------------------------------------------------------

class RegrooveFXUI : public UI
{
public:
    RegrooveFXUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        std::memset(fParameters, 0, sizeof(fParameters));

        // Create ImGui widget
        fImGuiWidget = new RegrooveFXImGuiWidget(this);
        fImGuiWidget->setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }

    ~RegrooveFXUI() override
    {
        delete fImGuiWidget;
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        fParameters[index] = value;
        fImGuiWidget->repaint();
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
    friend class RegrooveFXImGuiWidget;

    class RegrooveFXImGuiWidget : public ImGuiSubWidget
    {
    public:
        explicit RegrooveFXImGuiWidget(RegrooveFXUI* const ui)
            : ImGuiSubWidget(ui),
              fUI(ui)
        {
            // Setup Regroove style (from framework-agnostic UI)
            RegrooveFX::UI::setupStyle();
        }

    protected:
        void onImGuiDisplay() override
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

            if (ImGui::Begin("RegrooveFX", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoScrollbar))
            {
                // Call framework-agnostic render function
                // This same function can be used in SDL applications!
                if (RegrooveFX::UI::render(
                    // Distortion
                    &fUI->fParameters[kParameterDistortionEnabled],
                    &fUI->fParameters[kParameterDistortionDrive],
                    &fUI->fParameters[kParameterDistortionMix],
                    // Filter
                    &fUI->fParameters[kParameterFilterEnabled],
                    &fUI->fParameters[kParameterFilterCutoff],
                    &fUI->fParameters[kParameterFilterResonance],
                    // EQ
                    &fUI->fParameters[kParameterEQEnabled],
                    &fUI->fParameters[kParameterEQLow],
                    &fUI->fParameters[kParameterEQMid],
                    &fUI->fParameters[kParameterEQHigh],
                    // Compressor
                    &fUI->fParameters[kParameterCompressorEnabled],
                    &fUI->fParameters[kParameterCompressorThreshold],
                    &fUI->fParameters[kParameterCompressorRatio],
                    // Delay
                    &fUI->fParameters[kParameterDelayEnabled],
                    &fUI->fParameters[kParameterDelayTime],
                    &fUI->fParameters[kParameterDelayFeedback],
                    &fUI->fParameters[kParameterDelayMix],
                    // Layout
                    getWidth(), getHeight(), true))
                {
                    // Parameters changed - notify plugin
                    for (uint32_t i = 0; i < kParameterCount; i++) {
                        fUI->setParameterValue(i, fUI->fParameters[i]);
                    }
                }
            }
            ImGui::End();

            ImGui::PopStyleVar(2);
        }

    private:
        RegrooveFXUI* const fUI;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveFXImGuiWidget)
    };

    RegrooveFXImGuiWidget* fImGuiWidget;
    float fParameters[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveFXUI)
};

// -----------------------------------------------------------------------

UI* createUI()
{
    return new RegrooveFXUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
