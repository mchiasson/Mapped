#include "imgui.h"
#include "globals.h"
#include "editor.h"

void editor_quit();

void menuBar_updateGUI()
{
    bool showAboutPopup = false;

    if (ImGui::BeginMainMenuBar())
    {
        const char* activityIndicator[] = { "/", "-", "\\", "|" };
        static int indicatorIndex = 0;
        indicatorIndex = (indicatorIndex + 1) % (sizeof(activityIndicator) / sizeof(char*));
        ImGui::Text(activityIndicator[indicatorIndex]);

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Map", "CTRL + N")) { updateNextFrame++; editor_new(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Open Map", "CTRL + O")) { updateNextFrame++; editor_open(); }
            if (ImGui::BeginMenu("Recent Maps"))
            {
                for (const auto& recentPath : recentMaps)
                {
                    if (!recentPath.empty())
                    {
                        if (ImGui::MenuItem(recentPath.c_str())) { updateNextFrame++; editor_openRecent(recentPath); }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Map", "CTRL + S")) { updateNextFrame++; editor_save(); }
            if (ImGui::MenuItem("Save Map As", "CTRL + SHIFT + S")) { updateNextFrame++; editor_saveAs(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "ALT + F4")) { updateNextFrame++; editor_quit(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL + Z")) { updateNextFrame++; }
            if (ImGui::MenuItem("Redo", "CTRL + SHIFT + Z", false, false)) { updateNextFrame++; }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL + X")) { updateNextFrame++; }
            if (ImGui::MenuItem("Copy", "CTRL + C")) { updateNextFrame++; }
            if (ImGui::MenuItem("Paste", "CTRL + V")) { updateNextFrame++; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Left Panel", "B", &isLeftPanelVisible)) { updateNextFrame++; }
            if (ImGui::MenuItem("Right Panel", "N", &isRightPanelVisible)) { updateNextFrame++; }
            ImGui::Separator();
            if (ImGui::MenuItem("Full View", "ALT + W", &isFullView)) { updateNextFrame++; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                showAboutPopup = true;
                updateNextFrame += 30;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // About popup
    if (showAboutPopup)
    {
        ImGui::OpenPopup("About##Popup");
    }
    if (ImGui::BeginPopup("About##Popup", ImGuiWindowFlags_Modal))
    {
        ImGui::Text("Made by David St-Louis.");
        ImGui::Text("Copyright (C) David St-Louis 2018");
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
            updateNextFrame++;
        }
        ImGui::EndPopup();
    }
}
