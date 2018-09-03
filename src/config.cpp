#include "globals.h"

#include <json/json.h>
#include <fstream>
#include <algorithm>

#include <tinyfiledialogs.h>

#include <SDL.h>

#if defined(WIN32)
#include <Windows.h>
#include <Shlobj.h>

static std::string getFilename()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
    {
        std::string filename = szPath;
        filename += "\\Mapped";

        DWORD dwAttrib = GetFileAttributes(filename.c_str());
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            CreateDirectory(filename.c_str(), NULL);
        }

        std::replace(filename.begin(), filename.end(), '\\', '/');
        return filename + "/config.json";
    }
    else
    {
        // Try local..
        return "./config.json";
    }
}
#else
static std::string getFilename()
{
    // Try local..
    return "./config.json";
}
#endif

void config_load()
{
    auto filename = getFilename();

    Json::Value config;
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            // Ignore error silently
            return;
        }
        file >> config;
    }

    // Recents
    const auto& recent = config["recents"];
    recentMaps.clear();
    for (int i = 0; i < (int)recent.size(); ++i)
    {
        recentMaps.push_back(recent[i].asString());
    }

    // Window pos/size
    SDL_Window* pWindow = SDL_GL_GetCurrentWindow();
    int x = config["window"]["x"].asInt();
    int y = config["window"]["y"].asInt();
    int w = config["window"]["width"].asInt();
    int h = config["window"]["height"].asInt();
    if (x && y)
    {
        SDL_SetWindowPosition(pWindow, x, y);
    }
    if (w && h)
    {
        SDL_SetWindowSize(pWindow, w, h);
    }
    if (config["window"]["maximized"].asBool()) SDL_MaximizeWindow(pWindow);
}

void config_save()
{
    auto filename = getFilename();

    // Recents
    Json::Value recents(Json::ValueType::arrayValue);
    for (const auto& recent : recentMaps)
    {
        recents.append(recent);
    }

    // Window pos/dim/maximize
    SDL_Window* pWindow = SDL_GL_GetCurrentWindow();
    Json::Value window;
    int x, y;
    int w, h;
    SDL_GetWindowPosition(pWindow, &x, &y);
    SDL_GetWindowSize(pWindow, &w, &h);

    window["maximized"] = (SDL_GetWindowFlags(pWindow) & SDL_WINDOW_MAXIMIZED) ? true : false;
    window["x"] = x;
    window["y"] = y;
    window["width"] = w;
    window["height"] = h;

    // Configs
    Json::Value config;
    config["recents"] = recents;
    config["window"] = window;

    std::ofstream file(filename);
    if (!file.is_open())
    {
        tinyfd_messageBox("Config", "Error saving config file.\nThis shouldn't happen, contact app creator.", "ok", "error", 0);
        return;
    }
    file << config;
}
