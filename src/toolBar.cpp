#include "globals.h"
#include <imgui.h>

void toolBar_update()
{
    ImGui::SetNextWindowPos({ 0, 20 });
    ImGui::SetNextWindowSize({ (float)width, 32 });
    ImGui::Begin("Tool Bar", 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar);
    ImGui::End();
}
