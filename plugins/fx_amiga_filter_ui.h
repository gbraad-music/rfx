/**
 * Amiga Filter UI Helper
 * ImGui rendering for fx_amiga_filter
 */

#ifndef FX_AMIGA_FILTER_UI_H
#define FX_AMIGA_FILTER_UI_H

#include "../effects/fx_amiga_filter.h"
#include "DearImGuiKnobs/imgui-knobs.h"
#include "DearImGuiToggle/imgui_toggle.h"

namespace AmigaFilterUI {

static const char* filter_type_names[] = {
    "Off",
    "A500 (4.9kHz RC)",
    "A500 + LED (3.3kHz)",
    "A1200 (32kHz RC)",
    "A1200 + LED (3.3kHz)",
    "Unfiltered"
};

/**
 * Render Amiga filter controls
 * Returns true if any parameter changed
 */
static inline bool render(FXAmigaFilter* fx, float width = 300.0f, bool compact = false) {
    if (!fx) return false;

    bool changed = false;
    int enabled = fx_amiga_filter_get_enabled(fx);
    AmigaFilterType type = fx_amiga_filter_get_type(fx);
    float mix = fx_amiga_filter_get_mix(fx);

    if (compact) {
        // Compact layout - single row
        ImGui::BeginGroup();

        if (ImGui::ToggleButton("##amiga_enable", &enabled)) {
            fx_amiga_filter_set_enabled(fx, enabled);
            changed = true;
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(width * 0.5f);
        int type_idx = (int)type;
        if (ImGui::Combo("##amiga_type", &type_idx, filter_type_names, 6)) {
            fx_amiga_filter_set_type(fx, (AmigaFilterType)type_idx);
            changed = true;
        }
        ImGui::SameLine();

        if (ImGuiKnobs::Knob("Mix##amiga", &mix, 0.0f, 1.0f, 0.01f, "%.0f%%", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_amiga_filter_set_mix(fx, mix);
            changed = true;
        }

        ImGui::EndGroup();
    } else {
        // Full layout - vertical
        ImGui::BeginGroup();
        ImGui::Text("Amiga Filter");
        ImGui::Spacing();

        if (ImGui::Checkbox("Enable##amiga", (bool*)&enabled)) {
            fx_amiga_filter_set_enabled(fx, enabled);
            changed = true;
        }

        ImGui::SetNextItemWidth(width);
        int type_idx = (int)type;
        if (ImGui::Combo("Type##amiga", &type_idx, filter_type_names, 6)) {
            fx_amiga_filter_set_type(fx, (AmigaFilterType)type_idx);
            changed = true;
        }

        if (ImGuiKnobs::Knob("Mix##amiga", &mix, 0.0f, 1.0f, 0.01f, "%.0f%%", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_ValueTooltip)) {
            fx_amiga_filter_set_mix(fx, mix);
            changed = true;
        }

        ImGui::EndGroup();
    }

    return changed;
}

} // namespace AmigaFilterUI

#endif // FX_AMIGA_FILTER_UI_H
