/**
 * Paula BLEP UI Helper
 * ImGui rendering for fx_paula_blep
 */

#ifndef FX_PAULA_BLEP_UI_H
#define FX_PAULA_BLEP_UI_H

#include "../effects/fx_paula_blep.h"
#include "DearImGuiKnobs/imgui-knobs.h"
#include "DearImGuiToggle/imgui_toggle.h"

namespace PaulaBlepUI {

static const char* mode_names[] = {
    "A500 (4.9kHz)",
    "A500+LED (3.3kHz)",
    "A1200 (32kHz)",
    "A1200+LED (3.3kHz)",
    "Unfiltered"
};

/**
 * Render Paula BLEP controls
 * Returns true if any parameter changed
 */
static inline bool render(FXPaulaBlep* fx, float width = 300.0f, bool compact = false) {
    if (!fx) return false;

    bool changed = false;
    int enabled = fx_paula_blep_get_enabled(fx);
    PaulaMode mode = fx_paula_blep_get_mode(fx);
    float mix = fx_paula_blep_get_mix(fx);

    if (compact) {
        // Compact layout - single row
        ImGui::BeginGroup();

        if (ImGui::ToggleButton("##paula_enable", &enabled)) {
            fx_paula_blep_set_enabled(fx, enabled);
            changed = true;
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(width * 0.5f);
        int mode_idx = (int)mode;
        if (ImGui::Combo("##paula_mode", &mode_idx, mode_names, PAULA_NUM_MODES)) {
            fx_paula_blep_set_mode(fx, (PaulaMode)mode_idx);
            changed = true;
        }
        ImGui::SameLine();

        if (ImGuiKnobs::Knob("Mix##paula", &mix, 0.0f, 1.0f, 0.01f, "%.0f%%", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_paula_blep_set_mix(fx, mix);
            changed = true;
        }

        ImGui::EndGroup();
    } else {
        // Full layout - vertical
        ImGui::BeginGroup();
        ImGui::Text("Paula BLEP");
        ImGui::Spacing();

        if (ImGui::Checkbox("Enable##paula", (bool*)&enabled)) {
            fx_paula_blep_set_enabled(fx, enabled);
            changed = true;
        }

        ImGui::SetNextItemWidth(width);
        int mode_idx = (int)mode;
        if (ImGui::Combo("Mode##paula", &mode_idx, mode_names, PAULA_NUM_MODES)) {
            fx_paula_blep_set_mode(fx, (PaulaMode)mode_idx);
            changed = true;
        }

        if (ImGuiKnobs::Knob("Mix##paula", &mix, 0.0f, 1.0f, 0.01f, "%.0f%%", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_paula_blep_set_mix(fx, mix);
            changed = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Amiga Paula DAC emulation");
            ImGui::Text("Uses BLEP synthesis for authentic sound");
            ImGui::Text("A500: Warmer, more filtered");
            ImGui::Text("A1200: Brighter, less filtered");
            ImGui::Text("+LED: Additional 3.3kHz lowpass");
            ImGui::EndTooltip();
        }

        ImGui::EndGroup();
    }

    return changed;
}

} // namespace PaulaBlepUI

#endif // FX_PAULA_BLEP_UI_H
