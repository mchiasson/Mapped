#include "globals.h"
#include <imgui.h>

float pos[3] = { 0, 0, 0 };
float rot[3] = { 0, 0, 0 };
float scale[3] = { 1, 1, 1 };

void properties_updateGUI()
{
    if (!isLeftPanelVisible) return;

    auto totalH = (float)height - 52;

    {
        ImGui::SetNextWindowPos({ 0, 52 });
        ImGui::SetNextWindowSize({ PANEL_WIDTH, totalH / 2.0f });
        ImGui::Begin("Properties", 0,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);
        ImGui::DragFloat3("Position", pos, 0.10f);
        ImGui::DragFloat3("Rotation", rot, 1.0f);
        ImGui::DragFloat3("Scale", scale, 0.01f);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos({ 0, 52 + totalH / 2.0f });
        ImGui::SetNextWindowSize({ PANEL_WIDTH, totalH / 2.0f });
        ImGui::Begin("Custom", 0,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);
        ImGui::End();
    }
}
