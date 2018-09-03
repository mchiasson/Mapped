#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

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

void view_updateGUI(ViewType type, ViewLayout layout, int viewIndex);

#endif
