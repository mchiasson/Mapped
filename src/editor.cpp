#include "imgui.h"
#include "globals.h"
#include "menuBar.h"
#include "toolBar.h"
#include "properties.h"
#include "view.h"
#include "models.h"
#include "layers.h"

#include <stdlib.h>

void editor_update()
{
    // Some UI styling
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;

    menuBar_update();
    toolBar_update();
    properties_update();
    layers_update();
    models_update();

    if (isFullView)
    {
        view_update(ViewType::Perspective, ViewLayout::Full, 0);
    }
    else
    {
        view_update(ViewType::Perspective, ViewLayout::Four, 0);
        view_update(ViewType::Top, ViewLayout::Four, 1);
        view_update(ViewType::Left, ViewLayout::Four, 2);
        view_update(ViewType::Front, ViewLayout::Four, 3);
    }
}

void editor_quit()
{
    // Save
    extern bool done;
    done = true;
}
