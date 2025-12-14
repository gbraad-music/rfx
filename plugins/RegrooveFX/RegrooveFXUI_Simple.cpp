/*
 * RegrooveFX Plugin UI (Simple - No ImGui)
 * Copyright (C) 2024
 */

#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

class RegrooveFXUI : public UI
{
    float fParameters[kParameterCount];

public:
    RegrooveFXUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        std::memset(fParameters, 0, sizeof(fParameters));
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        fParameters[index] = value;
        repaint();
    }

    void onDisplay() override
    {
        // Simple placeholder UI
        // Real UI with ImGui/DearImGui will be added later
    }
};

UI* createUI()
{
    return new RegrooveFXUI();
}

END_NAMESPACE_DISTRHO
