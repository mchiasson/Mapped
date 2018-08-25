#include "globals.h"
#include <imgui.h>

void models_update()
{
    if (!isRightPanelVisible) return;

    auto totalH = (float)height - 52;
    auto hTop = totalH * 1.0f / 3.0f;
    auto hBottom = totalH * 2.0f / 3.0f;

    ImGui::SetNextWindowPos({ (float)width - PANEL_WIDTH, 52 + hTop });
    ImGui::SetNextWindowSize({ PANEL_WIDTH, hBottom });
    ImGui::Begin("Models", 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);
    ImGui::End();
}
