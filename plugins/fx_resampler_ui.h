/**
 * Resampler UI Helper
 * ImGui rendering for fx_resampler
 */

#ifndef FX_RESAMPLER_UI_H
#define FX_RESAMPLER_UI_H

#include "../effects/fx_resampler.h"
#include "DearImGuiKnobs/imgui-knobs.h"
#include "DearImGuiToggle/imgui_toggle.h"

namespace ResamplerUI {

static const char* mode_names[] = {
    "Nearest (Zero-Hold)",
    "Linear (2-point)",
    "Cubic (4-point Sinc)",
    "Sinc8 (8-point AA)"
};

/**
 * Render resampler controls
 * Returns true if any parameter changed
 */
static inline bool render(FXResampler* fx, float width = 300.0f, bool compact = false) {
    if (!fx) return false;

    bool changed = false;
    int enabled = fx_resampler_get_enabled(fx);
    ResamplerMode mode = fx_resampler_get_mode(fx);
    float rate = fx_resampler_get_rate(fx);

    if (compact) {
        // Compact layout - single row
        ImGui::BeginGroup();

        if (ImGui::ToggleButton("##resamp_enable", &enabled)) {
            fx_resampler_set_enabled(fx, enabled);
            changed = true;
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(width * 0.4f);
        int mode_idx = (int)mode;
        if (ImGui::Combo("##resamp_mode", &mode_idx, mode_names, RESAMPLER_NUM_MODES)) {
            fx_resampler_set_mode(fx, (ResamplerMode)mode_idx);
            changed = true;
        }
        ImGui::SameLine();

        if (ImGuiKnobs::Knob("Rate##resamp", &rate, 0.25f, 4.0f, 0.01f, "%.2fx", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_resampler_set_rate(fx, rate);
            changed = true;
        }

        ImGui::EndGroup();
    } else {
        // Full layout - vertical
        ImGui::BeginGroup();
        ImGui::Text("Resampler");
        ImGui::Spacing();

        if (ImGui::Checkbox("Enable##resamp", (bool*)&enabled)) {
            fx_resampler_set_enabled(fx, enabled);
            changed = true;
        }

        ImGui::SetNextItemWidth(width);
        int mode_idx = (int)mode;
        if (ImGui::Combo("Mode##resamp", &mode_idx, mode_names, RESAMPLER_NUM_MODES)) {
            fx_resampler_set_mode(fx, (ResamplerMode)mode_idx);
            changed = true;
        }

        if (ImGuiKnobs::Knob("Rate##resamp", &rate, 0.25f, 4.0f, 0.01f, "%.2fx", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_resampler_set_rate(fx, rate);
            changed = true;
        }

        ImGui::SameLine();
        ImGui::Text("(0.25x - 4.0x)");

        ImGui::EndGroup();
    }

    return changed;
}

} // namespace ResamplerUI

#endif // FX_RESAMPLER_UI_H
