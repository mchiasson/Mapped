#include "imgui.h"
#include "globals.h"
#include "menuBar.h"
#include "toolBar.h"
#include "properties.h"
#include "view.h"
#include "models.h"
#include "layers.h"
#include "editor.h"
#include "config.h"

#include <tinyfiledialogs.h>

#include <stdlib.h>
#include <fstream>

void editor_init()
{
    config_load();

    if (recentMaps.empty())
    {
        editor_new();
    }
    else
    {
        editor_openRecent(recentMaps[0]);
    }
}

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

void editor_new()
{
    // Clear
    document = Document();
    document.dirty = false;

    // Editor specific stuff
    Json::Value views;
    {
        Json::Value position;
        position["x"] = -2;
        position["y"] = -2;
        position["z"] = 2;

        Json::Value perspective;
        perspective["position"] = position;
        perspective["angleX"] = -30;
        perspective["angleZ"] = 45;

        views["perspective"] = perspective;
    }
    {
        Json::Value position;
        position["x"] = 0;
        position["y"] = 0;
        position["z"] = 0;

        Json::Value top;
        top["position"] = position;
        top["zoom"] = 18;

        views["top"] = top;
    }
    {
        Json::Value position;
        position["x"] = 0;
        position["y"] = 0;
        position["z"] = 0;

        Json::Value left;
        left["position"] = position;
        left["zoom"] = 18;

        views["left"] = left;
    }
    {
        Json::Value position;
        position["x"] = 0;
        position["y"] = 0;
        position["z"] = 0;

        Json::Value front;
        front["position"] = position;
        front["zoom"] = 18;

        views["front"] = front;
    }

    Json::Value editor;
    editor["fullView"] = false;
    editor["leftPanel"] = true;
    editor["rightPanel"] = true;
    editor["snap"] = true;
    editor["views"] = views;

    // model library
    Json::Value library(Json::ValueType::arrayValue);

    // entities
    Json::Value map(Json::ValueType::arrayValue);

    document.json["version"] = MAP_VERSION;
    document.json["editor"] = editor;
    document.json["library"] = library;
    document.json["map"] = map;

    view_load();
}

static void addRecent(const std::string& filename)
{
    // Remove if previously there
    for (auto it = recentMaps.begin(); it != recentMaps.end(); ++it)
    {
        if (*it == document.filename)
        {
            recentMaps.erase(it);
            break;
        }
    }

    // Push front
    recentMaps.insert(recentMaps.begin(), document.filename);

    // Pop olders
    while (recentMaps.size() > MAX_RECENT_MAPS) recentMaps.pop_back();
}

static void open(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        tinyfd_messageBox("Open", ("Failed to open file:\n" + filename).c_str(), "ok", "error", 0);
        return;
    }
    file >> document.json;

    document.dirty = false;
    document.filename = filename;

    addRecent(document.filename);

    view_load();
}

static void save()
{
    std::ofstream file(document.filename);
    if (!file.is_open())
    {
        tinyfd_messageBox("Open", ("Failed to open/create file:\n" + document.filename).c_str(), "ok", "error", 0);
        return;
    }
    file << document.json;

    document.dirty = false;

    // Add to recent
    addRecent(document.filename);
}

void editor_open()
{
    if (document.dirty)
    {
        switch (tinyfd_messageBox("Open", "Your map wasn't save. Save now?", "yesnocancel", "warning", 0))
        {
        case 0: return;
        case 1: editor_save(); break;
        }
    }

    const char* filters[] = { "*.json" };
    auto ret = tinyfd_openFileDialog("Open", "", 1, filters, NULL, 0);
    if (!ret) return;
    open(ret);
}

void editor_openRecent(const std::string& filename)
{
    if (document.dirty)
    {
        switch (tinyfd_messageBox("Open", "Your map wasn't save. Save now?", "yesnocancel", "warning", 0))
        {
        case 0: return;
        case 1: editor_save(); break;
        }
    }

    open(filename);
}

void editor_save()
{
    if (document.dirty || document.filename.empty())
    {
        if (document.filename.empty())
        {
            editor_saveAs();
            return;
        }

        save();
    }
}

void editor_saveAs()
{
    if (!document.filename.empty() && document.dirty)
    {
        switch (tinyfd_messageBox("Save As", "Your map wasn't save. Save now?", "yesnocancel", "warning", 0))
        {
            case 0: return;
            case 1: editor_save(); break;
        }
    }

    const char* filters[] = { "*.json" };
    auto ret = tinyfd_saveFileDialog("Save As", "untitled.json", 1, filters, NULL);
    if (!ret) return;
    document.filename = ret;
    if (document.filename.size() < 5 || document.filename.substr(document.filename.size() - 5) != ".json")
    {
        document.filename += ".json";
    }

    save();
}
