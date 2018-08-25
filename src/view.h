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

void view_update(ViewType type, ViewLayout layout, int viewIndex);
