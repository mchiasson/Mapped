#include "view.h"
#include "globals.h"
#include <imgui.h>

const char* VIEW_TYPE_TO_NAME[] = {
    "Perspective",
    "Top",
    "Left",
    "Right",
    "Bottom",
    "Front",
    "Back"
};

void view_update(ViewType type, ViewLayout layout, int viewIndex)
{
    auto x = 0.0f;
    auto y = 52.0f;
    auto w = (float)width;
    auto h = (float)height - y;

    if (isLeftPanelVisible)
    {
        x += PANEL_WIDTH;
        w -= PANEL_WIDTH;
    }
    if (isRightPanelVisible) w -= PANEL_WIDTH;

    if (layout == ViewLayout::Four)
    {
        w /= 2.0f;
        h /= 2.0f;
        if (viewIndex & 1) x += w;
        if (viewIndex >= 2) y += h;
    }

    ImGui::SetNextWindowPos({ x, y });
    ImGui::SetNextWindowSize({ w, h });
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin(VIEW_TYPE_TO_NAME[(int)type], 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);
    ImGui::End();
}
