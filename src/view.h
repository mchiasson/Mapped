#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

#define MAX_VIEWS 4

enum class ViewType : int
{
    Perspective = 0,
    Top,
    Left,
    Right,
    Bottom,
    Front,
    Back
};

enum class ViewLayout : int
{
    Four = 0,
    Full
};

struct ViewInfo
{
    float position[3] = { 0, 0, 0 };
    int zoomLevel = 18;
    float angleX = 0.0f;
    float angleZ = 0.0f;
    ViewType type;
    float x, y;
    float w, h;
    int index;
};

extern const char* VIEW_TYPE_TO_NAME[];
extern ViewInfo viewInfos[MAX_VIEWS];

void view_updateGUI(ViewType type, ViewLayout layout, int viewIndex);
void view_load();

#endif
