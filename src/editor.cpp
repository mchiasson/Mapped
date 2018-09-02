#include "imgui.h"
#include "globals.h"
#include "menuBar.h"
#include "toolBar.h"
#include "properties.h"
#include "view.h"
#include "models.h"
#include "layers.h"

#include <stdlib.h>

void editor_updateGUI()
{
    // Some UI styling
    auto& style = ImGui::GetStyle();
    //style.WindowRounding = 0.0f;

    menuBar_updateGUI();
    toolBar_updateGUI();
    properties_updateGUI();
    layers_updateGUI();
    models_updateGUI();

    // Prepare the data
    

    if (isFullView)
    {
        view_updateGUI(ViewType::Perspective, ViewLayout::Full, 0);
    }
    else
    {
        view_updateGUI(ViewType::Perspective, ViewLayout::Four, 0);
        view_updateGUI(ViewType::Top, ViewLayout::Four, 1);
        view_updateGUI(ViewType::Left, ViewLayout::Four, 2);
        view_updateGUI(ViewType::Front, ViewLayout::Four, 3);
    }
}

void editor_quit()
{
    // Save
    extern bool done;
    done = true;
}
