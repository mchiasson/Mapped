#include "imgui.h"
#include "globals.h"

void editor_quit();

void menuBar_updateGUI()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "ALT+F4"))
            {
                editor_quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Left Panel", "B", &isLeftPanelVisible);
            ImGui::MenuItem("Right Panel", "N", &isRightPanelVisible);
            ImGui::Separator();
            ImGui::MenuItem("Full View", "F", &isFullView);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
