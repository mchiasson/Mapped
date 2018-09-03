#include "globals.h"

bool isLeftPanelVisible = true;
bool isRightPanelVisible = true;
bool isFullView = false;

int width = 1280;
int height = 720;

int updateNextFrame = 0;
bool updateEachFrame = false;

std::vector<std::string> recentMaps;

Document document;
