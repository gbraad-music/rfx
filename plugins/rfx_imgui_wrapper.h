/*
 * RFX ImGui Wrapper for DPF
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef RFX_IMGUI_WRAPPER_H
#define RFX_IMGUI_WRAPPER_H

#include "imgui.h"
#include "DistrhoUI.hpp"

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <GL/gl.h>
#elif defined(__APPLE__)
    #include <OpenGL/gl.h>
    #include <time.h>
#else
    #include <GL/gl.h>
    #include <time.h>
#endif

START_NAMESPACE_DISTRHO

/**
 * Base class for DPF UIs that use ImGui
 * Handles ImGui context initialization and OpenGL backend
 */
class ImGuiUI : public UI
{
public:
    ImGuiUI(uint width, uint height)
        : UI(width, height)
    {
        // Create ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Initialize OpenGL backend (manual, since DPF doesn't provide this)
        fFontTexture = 0;
        unsigned char* pixels;
        int width_px, height_px;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width_px, &height_px);
        
        glGenTextures(1, &fFontTexture);
        glBindTexture(GL_TEXTURE_2D, fFontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_px, height_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        
        io.Fonts->SetTexID((ImTextureID)(intptr_t)fFontTexture);
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        fTime = 0.0;
    }

    ~ImGuiUI() override
    {
        if (fFontTexture) {
            glDeleteTextures(1, &fFontTexture);
            fFontTexture = 0;
        }
        
        ImGui::DestroyContext();
    }

protected:
    void onDisplay() override
    {
        ImGuiIO& io = ImGui::GetIO();
        
        // Update display size
        io.DisplaySize = ImVec2((float)getWidth(), (float)getHeight());
        
        // Update time
        double current_time = getCurrentTime();
        io.DeltaTime = fTime > 0.0 ? (float)(current_time - fTime) : (float)(1.0f/60.0f);
        fTime = current_time;
        
        // Start frame
        ImGui::NewFrame();
        
        // Render UI (to be implemented by subclass)
        renderImGui();
        
        // Render
        ImGui::Render();
        renderDrawData(ImGui::GetDrawData());
    }
    
    virtual void renderImGui() = 0;

private:
    GLuint fFontTexture;
    double fTime;
    
    double getCurrentTime() const
    {
        #if defined(_WIN32)
            LARGE_INTEGER frequency, counter;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&counter);
            return (double)counter.QuadPart / (double)frequency.QuadPart;
        #else
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
        #endif
    }
    
    void renderDrawData(ImDrawData* draw_data)
    {
        // Save OpenGL state
        GLint last_viewport[4];
        glGetIntegerv(GL_VIEWPORT, last_viewport);
        
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        
        // Setup render state
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_TEXTURE_2D);
        
        // Setup viewport, orthographic projection matrix
        glViewport(0, 0, (GLsizei)draw_data->DisplaySize.x, (GLsizei)draw_data->DisplaySize.y);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0f, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, -1.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        // Render command lists
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
            const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
            
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            
            glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)((char*)vtx_buffer + offsetof(ImDrawVert, pos)));
            glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)((char*)vtx_buffer + offsetof(ImDrawVert, uv)));
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)((char*)vtx_buffer + offsetof(ImDrawVert, col)));
            
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
                    glScissor((int)pcmd->ClipRect.x,
                             (int)(draw_data->DisplaySize.y - pcmd->ClipRect.w),
                             (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                             (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, 
                                 sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 
                                 idx_buffer);
                }
                idx_buffer += pcmd->ElemCount;
            }
            
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }
        
        // Restore modified GL state
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        
        glDisable(GL_SCISSOR_TEST);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    }
    
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiUI)
};

END_NAMESPACE_DISTRHO

#endif // RFX_IMGUI_WRAPPER_H
