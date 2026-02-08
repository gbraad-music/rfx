/**
 * Regroove UI Components
 * Shared ImGui widgets for Regroovelizer, Junglizer, etc.
 */

#ifndef REGROOVE_UI_H
#define REGROOVE_UI_H

#include <imgui.h>

namespace RegroovelizerUI {

// Size constants
namespace Size {
    static const float FaderWidth   = 50.0f;
    static const float FaderHeight  = 200.0f;
    static const float ButtonHeight = 30.0f;
    static const float Spacing      = 12.0f;
    static const float PanelPadding = 20.0f;
}

// Color palette - ImVec4 format for ImGui styles
namespace Colors {
    // Signature Regroove Red
    static const ImVec4 Red         = ImVec4(0.81f, 0.10f, 0.22f, 1.0f);  // #CF1A37
    static const ImVec4 RedActive   = ImVec4(0.91f, 0.20f, 0.32f, 1.0f);
    static const ImVec4 RedDark     = ImVec4(0.71f, 0.05f, 0.17f, 1.0f);

    // Backgrounds
    static const ImVec4 Black       = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);  // #0A0A0A
    static const ImVec4 Dark        = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);  // #1A1A1A
    static const ImVec4 KnobOuter   = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);  // #2A2A2A
    static const ImVec4 KnobCap     = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);  // #555555
    static const ImVec4 FaderBg     = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

    // Text
    static const ImVec4 Gold        = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
    static const ImVec4 Text        = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    static const ImVec4 TextDim     = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
}

// Render centered title text
inline void renderTitle(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Gold);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

// Render vertical fader (like mixer fader)
// Returns true if value changed
inline bool renderFader(const char* label, float* value, float min_val = 0.0f, float max_val = 1.0f,
                        float width = 50.0f, float height = 200.0f) {
    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::FaderBg);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, Colors::Red);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Colors::RedActive);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, width - 4.0f);

    char id[64];
    snprintf(id, sizeof(id), "##fader_%s", label);
    bool changed = ImGui::VSliderFloat(id, ImVec2(width, height), value, min_val, max_val, "");

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Label below
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();

    ImGui::EndGroup();
    return changed;
}

// Render XY pad for touch control (EXACT original implementation)
// Returns true if value changed
inline bool renderXYPad(float* x, float* y, bool* touching, float width = 400.0f) {
    // 4:3 aspect ratio
    float height = width * 0.75f;

    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImVec2(width, height);
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background (dark gray)
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(26, 26, 26, 255), 3.0f);

    // Grid lines (subtle)
    const int grid_lines = 4;
    for (int i = 1; i < grid_lines; i++) {
        float x_pos = canvas_p0.x + (width / grid_lines) * i;
        float y_pos = canvas_p0.y + (height / grid_lines) * i;
        draw_list->AddLine(ImVec2(x_pos, canvas_p0.y), ImVec2(x_pos, canvas_p1.y),
                          IM_COL32(40, 40, 40, 255), 1.0f);
        draw_list->AddLine(ImVec2(canvas_p0.x, y_pos), ImVec2(canvas_p1.x, y_pos),
                          IM_COL32(40, 40, 40, 255), 1.0f);
    }

    // Center crosshair
    float center_x = canvas_p0.x + width * 0.5f;
    float center_y = canvas_p0.y + height * 0.5f;
    draw_list->AddLine(ImVec2(center_x - 10, center_y), ImVec2(center_x + 10, center_y),
                      IM_COL32(85, 85, 85, 255), 1.0f);
    draw_list->AddLine(ImVec2(center_x, center_y - 10), ImVec2(center_x, center_y + 10),
                      IM_COL32(85, 85, 85, 255), 1.0f);

    // Border (red when active)
    ImU32 border_color = *touching ? IM_COL32(207, 26, 55, 255) : IM_COL32(85, 85, 85, 255);
    draw_list->AddRect(canvas_p0, canvas_p1, border_color, 3.0f, 0, 2.0f);

    // Handle touch input
    ImGui::InvisibleButton("xypad", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);

    bool changed = false;
    if (ImGui::IsItemActive()) {
        ImVec2 mouse_pos = io.MousePos;
        ImVec2 mouse_in_canvas(mouse_pos.x - canvas_p0.x, mouse_pos.y - canvas_p0.y);

        // Convert to 0-1023 range (Y-flipped, origin at bottom-left)
        float norm_x = mouse_in_canvas.x / canvas_sz.x;
        float norm_y = 1.0f - (mouse_in_canvas.y / canvas_sz.y);

        // Clamp
        norm_x = (norm_x < 0.0f) ? 0.0f : (norm_x > 1.0f) ? 1.0f : norm_x;
        norm_y = (norm_y < 0.0f) ? 0.0f : (norm_y > 1.0f) ? 1.0f : norm_y;

        *x = norm_x * 1023.0f;
        *y = norm_y * 1023.0f;
        *touching = true;
        changed = true;

        // Draw touch position (red circle with glow)
        ImVec2 touch_screen(canvas_p0.x + norm_x * canvas_sz.x,
                           canvas_p1.y - norm_y * canvas_sz.y);

        // Glow effect
        draw_list->AddCircleFilled(touch_screen, 16.0f, IM_COL32(207, 26, 55, 50));
        draw_list->AddCircleFilled(touch_screen, 12.0f, IM_COL32(207, 26, 55, 100));
        draw_list->AddCircleFilled(touch_screen, 8.0f, IM_COL32(207, 26, 55, 200));

        // Center dot
        draw_list->AddCircleFilled(touch_screen, 4.0f, IM_COL32(255, 255, 255, 255));

        // Crosshair at touch point
        draw_list->AddLine(ImVec2(canvas_p0.x, touch_screen.y),
                          ImVec2(canvas_p1.x, touch_screen.y),
                          IM_COL32(207, 26, 55, 100), 1.0f);
        draw_list->AddLine(ImVec2(touch_screen.x, canvas_p0.y),
                          ImVec2(touch_screen.x, canvas_p1.y),
                          IM_COL32(207, 26, 55, 100), 1.0f);

    } else if (*touching) {
        *touching = false;
        changed = true;
    }

    return changed;
}

/**
 * Apply Regroove style to ImGui
 */
inline void setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]         = Colors::Black;
    colors[ImGuiCol_ChildBg]          = Colors::Black;
    colors[ImGuiCol_Border]           = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg]          = Colors::FaderBg;
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Button]           = Colors::KnobOuter;
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.36f, 0.37f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.46f, 0.47f, 0.50f, 1.00f);
    colors[ImGuiCol_Text]             = Colors::Text;
    colors[ImGuiCol_SliderGrab]       = Colors::Red;
    colors[ImGuiCol_SliderGrabActive] = Colors::RedActive;

    style.WindowRounding   = 0.0f;
    style.FrameRounding    = 3.0f;
    style.GrabRounding     = 3.0f;
    style.ItemSpacing      = ImVec2(Size::Spacing, 8);
    style.WindowPadding    = ImVec2(Size::PanelPadding, Size::PanelPadding);
}

/**
 * Render red banner with hamburger menu and title
 *
 * @param title Application title (e.g., "REGROOVELIZER", "JUNGLIZER")
 * @param show_settings Pointer to settings visibility flag (toggled on hamburger click)
 * @param window_width Width of the window
 */
inline void renderBanner(const char* title, bool* show_settings, float window_width) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Red banner background (48px height)
    ImVec2 banner_min = ImVec2(0, 0);
    ImVec2 banner_max = ImVec2(window_width, 48);

    // Draw red background #CF1A37
    draw_list->AddRectFilled(banner_min, banner_max, IM_COL32(207, 26, 55, 255));

    // Draw shadow
    draw_list->AddRectFilledMultiColor(
        ImVec2(banner_min.x, banner_max.y),
        ImVec2(banner_max.x, banner_max.y + 2),
        IM_COL32(0, 0, 0, 128),
        IM_COL32(0, 0, 0, 128),
        IM_COL32(0, 0, 0, 0),
        IM_COL32(0, 0, 0, 0)
    );

    // Hamburger menu button (left edge of banner)
    ImVec2 button_pos = ImVec2(8, 8);
    ImVec2 button_size = ImVec2(48, 32);
    ImVec2 button_max = ImVec2(button_pos.x + button_size.x, button_pos.y + button_size.y);

    // Check if mouse is over button
    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool hovered = mouse_pos.x >= button_pos.x && mouse_pos.x <= button_max.x &&
                   mouse_pos.y >= button_pos.y && mouse_pos.y <= button_max.y;
    bool clicked = hovered && ImGui::IsMouseClicked(0);

    if (clicked && show_settings) {
        *show_settings = !(*show_settings);
    }

    // Draw button background
    if (hovered) {
        draw_list->AddRectFilled(button_pos, button_max, IM_COL32(255, 255, 255, 30), 4.0f);
    }

    // Draw hamburger icon (☰ - three horizontal lines)
    ImVec2 line_start = ImVec2(button_pos.x + 8, button_pos.y + 8);
    draw_list->AddRectFilled(line_start, ImVec2(line_start.x + 32, line_start.y + 3), IM_COL32(255, 255, 255, 255));
    line_start.y += 7;
    draw_list->AddRectFilled(line_start, ImVec2(line_start.x + 32, line_start.y + 3), IM_COL32(255, 255, 255, 255));
    line_start.y += 7;
    draw_list->AddRectFilled(line_start, ImVec2(line_start.x + 32, line_start.y + 3), IM_COL32(255, 255, 255, 255));

    // Title text in banner (after hamburger, white, uppercase)
    ImVec2 banner_text_pos = ImVec2(button_max.x + 12, 14);
    draw_list->AddText(banner_text_pos, IM_COL32(255, 255, 255, 255), title);
}

/**
 * Render parameter display
 */
inline void renderParamInfo(const char* name, int min, int max, int value) {
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::Text("%s", name);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::Text("[%d-%d]", min, max);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
    ImGui::Text("%d", value);
    ImGui::PopStyleColor();
}

} // namespace RegroovelizerUI

#endif // REGROOVE_UI_H
