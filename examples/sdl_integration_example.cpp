/*
 * RegrooveFX SDL Integration Example
 *
 * Shows how to use regroove_effects_ui.h in SDL+ImGui applications
 * (like ../regroove and ../samplecrate)
 *
 * The SAME UI code works in both:
 * - DPF plugins (RegrooveFXUI.cpp)
 * - SDL applications (this example)
 */

#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

// Include the framework-agnostic UI and DSP
#include "regroove_effects.h"
#include "regroove_effects_ui.h"

int main(int argc, char** argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "RegrooveFX - SDL Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1000, 450,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Apply Regroove style (from framework-agnostic UI)
    RegrooveFX::UI::setupStyle();

    // Create DSP engine
    RegrooveEffects* effects = regroove_effects_create();

    // Parameter storage (same as plugin)
    float parameters[20] = {0};

    // Initialize some default values
    parameters[4] = 0.8f;  // Filter cutoff
    parameters[5] = 0.3f;  // Filter resonance

    // Sync to DSP
    regroove_effects_set_filter_cutoff(effects, parameters[4]);
    regroove_effects_set_filter_resonance(effects, parameters[5]);

    // Main loop
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                quit = true;
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // === RENDER UI USING THE SAME CODE AS THE PLUGIN ===
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

            if (ImGui::Begin("RegrooveFX", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                // This is the EXACT same function used in the plugin!
                if (RegrooveFX::UI::render(
                    // Distortion
                    &parameters[0], &parameters[1], &parameters[2],
                    // Filter
                    &parameters[3], &parameters[4], &parameters[5],
                    // EQ
                    &parameters[6], &parameters[7], &parameters[8], &parameters[9],
                    // Compressor
                    &parameters[10], &parameters[11], &parameters[12],
                    // Delay
                    &parameters[13], &parameters[14], &parameters[15], &parameters[16],
                    // Layout
                    io.DisplaySize.x, io.DisplaySize.y, true))
                {
                    // Parameters changed - update DSP
                    regroove_effects_set_distortion_enabled(effects, parameters[0] >= 0.5f);
                    regroove_effects_set_distortion_drive(effects, parameters[1]);
                    regroove_effects_set_distortion_mix(effects, parameters[2]);

                    regroove_effects_set_filter_enabled(effects, parameters[3] >= 0.5f);
                    regroove_effects_set_filter_cutoff(effects, parameters[4]);
                    regroove_effects_set_filter_resonance(effects, parameters[5]);

                    regroove_effects_set_eq_enabled(effects, parameters[6] >= 0.5f);
                    regroove_effects_set_eq_low(effects, parameters[7]);
                    regroove_effects_set_eq_mid(effects, parameters[8]);
                    regroove_effects_set_eq_high(effects, parameters[9]);

                    regroove_effects_set_compressor_enabled(effects, parameters[10] >= 0.5f);
                    regroove_effects_set_compressor_threshold(effects, parameters[11]);
                    regroove_effects_set_compressor_ratio(effects, parameters[12]);

                    regroove_effects_set_delay_enabled(effects, parameters[13] >= 0.5f);
                    regroove_effects_set_delay_time(effects, parameters[14]);
                    regroove_effects_set_delay_feedback(effects, parameters[15]);
                    regroove_effects_set_delay_mix(effects, parameters[16]);
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 26, 26, 26, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    regroove_effects_destroy(effects);

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
