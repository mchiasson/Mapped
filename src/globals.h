#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#define PANEL_WIDTH 240.0f
#define MAX_RECENT_MAPS 10

#define MAP_VERSION 1

#include <json/json.h>
#include <string>
#include <vector>

struct Document
{
    Json::Value json;
    bool dirty;
    std::string filename;
};

extern bool isLeftPanelVisible;
extern bool isRightPanelVisible;
extern bool isFullView;

extern int width;
extern int height;

extern int updateNextFrame;
extern bool updateEachFrame;

extern std::vector<std::string> recentMaps;

extern Document document;

#endif
