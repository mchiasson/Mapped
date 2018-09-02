#include "globals.h"
#include <imgui.h>

void layers_updateGUI()
{
    if (!isRightPanelVisible) return;

    auto totalH = (float)height - 52;
    auto hTop = totalH * 1.0f / 3.0f;

    ImGui::SetNextWindowPos({ (float)width - PANEL_WIDTH, 52});
    ImGui::SetNextWindowSize({ PANEL_WIDTH, hTop });
    ImGui::Begin("Layers", 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);
    ImGui::End();
}
