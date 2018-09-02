#include "view.h"
#include "globals.h"
#include <imgui.h>
#include <stdio.h>
#include <cinttypes>
#include <algorithm>
#include <GL/gl3w.h>

#define MAX_VIEWS 4
#define GRID_2D_SIZE 101

static const float ZOOM_LEVELS[] = {
    0.0625f,
    0.09375f,
    0.125f,
    0.1875f,
    0.25f,
    0.375f,
    0.5f,
    0.75f,
    1.0f, // 1 pixel = 1 meter
    1.5f,
    2.0f,
    3.0f,
    4.0f,
    6.0f,
    8.0f,
    12.0f,
    16.0f,
    24.0f,
    32.0f, // Default
    48.0f,
    64.0f,
    96.0f,
    128.0f,
    192.0f,
    256.0f
};
static const int MAX_ZOOM_LEVELS = sizeof(ZOOM_LEVELS) / sizeof(float);

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

ViewInfo viewInfos[MAX_VIEWS] = {
    { { -2, -2, 2 }, 15, 30, 45 },
    { { 0, 0, 0 }, 18, 0, 0 },
    { { 0, 0, 0 }, 18, 0, 0 },
    { { 0, 0, 0 }, 18, 0, 0 }
};

int draggingView = -1;

const char* VIEW_TYPE_TO_NAME[] = {
    "Perspective",
    "Top",
    "Left",
    "Right",
    "Bottom",
    "Front",
    "Back"
};

static bool initialized = false;

static float viewPosOnDragStart[2] = { 0, 0 };

struct ShaderGrid2D
{
    GLuint program = 0;
    GLint uniform_worldMtx = 0;
    GLint uniform_projMtx = 0;
    GLint attrib_position = 0;
    GLint attrib_color = 0;
} shader_grid2D;

struct ShaderGrid3D
{
    GLuint program = 0;
    GLint uniform_projMtx = 0;
    GLint attrib_position = 0;
    GLint attrib_color = 0;
} shader_grid3D;

struct GridMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
};

GridMesh mesh_grid0, mesh_grid1, mesh_grid2, mesh_grid3;

struct Grid2DVertex
{
    float position[2];
    float color[4];
};

static void viewDrawCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);

struct GLStates
{
    // Backup GL state
    GLenum last_active_texture;
    GLint last_program;
    GLint last_texture;
    GLint last_sampler;
    GLint last_array_buffer;
    GLint last_vertex_array;
#ifdef GL_POLYGON_MODE
    GLint last_polygon_mode[2];
#endif
    GLint last_viewport[4];
    GLint last_scissor_box[4];
    GLenum last_blend_src_rgb;
    GLenum last_blend_dst_rgb;
    GLenum last_blend_src_alpha;
    GLenum last_blend_dst_alpha;
    GLenum last_blend_equation_rgb;
    GLenum last_blend_equation_alpha;
    GLboolean last_enable_blend;
    GLboolean last_enable_cull_face;
    GLboolean last_enable_depth_test;
    GLboolean last_enable_scissor_test;
};

static void saveGLStates(GLStates* pStates)
{
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&pStates->last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_CURRENT_PROGRAM, &pStates->last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &pStates->last_texture);
    glGetIntegerv(GL_SAMPLER_BINDING, &pStates->last_sampler);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &pStates->last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &pStates->last_vertex_array);
#ifdef GL_POLYGON_MODE
    glGetIntegerv(GL_POLYGON_MODE, pStates->last_polygon_mode);
#endif
    glGetIntegerv(GL_VIEWPORT, pStates->last_viewport);
    glGetIntegerv(GL_SCISSOR_BOX, pStates->last_scissor_box);
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&pStates->last_blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&pStates->last_blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&pStates->last_blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&pStates->last_blend_dst_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&pStates->last_blend_equation_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&pStates->last_blend_equation_alpha);
    pStates->last_enable_blend = glIsEnabled(GL_BLEND);
    pStates->last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    pStates->last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    pStates->last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
}

static void restoreGLStates(GLStates* pStates)
{
    glUseProgram(pStates->last_program);
    glBindTexture(GL_TEXTURE_2D, pStates->last_texture);
#ifdef GL_SAMPLER_BINDING
    glBindSampler(0, pStates->last_sampler);
#endif
    glActiveTexture(pStates->last_active_texture);
    glBindVertexArray(pStates->last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, pStates->last_array_buffer);
    glBlendEquationSeparate(pStates->last_blend_equation_rgb, pStates->last_blend_equation_alpha);
    glBlendFuncSeparate(pStates->last_blend_src_rgb, pStates->last_blend_dst_rgb, pStates->last_blend_src_alpha, pStates->last_blend_dst_alpha);
    if (pStates->last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (pStates->last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (pStates->last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (pStates->last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)pStates->last_polygon_mode[0]);
#endif
    glViewport(pStates->last_viewport[0], pStates->last_viewport[1], (GLsizei)pStates->last_viewport[2], (GLsizei)pStates->last_viewport[3]);
    glScissor(pStates->last_scissor_box[0], pStates->last_scissor_box[1], (GLsizei)pStates->last_scissor_box[2], (GLsizei)pStates->last_scissor_box[3]);
}

void view_updateGUI(ViewType type, ViewLayout layout, int viewIndex)
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
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin(VIEW_TYPE_TO_NAME[(int)type], 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);

    if (ImGui::IsMouseHoveringWindow())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel < -0.1f)
        {
            viewInfos[viewIndex].zoomLevel = 
                std::max(0, viewInfos[viewIndex].zoomLevel - 1);
        }
        else if (io.MouseWheel > 0.1f)
        {
            viewInfos[viewIndex].zoomLevel = 
                std::min(MAX_ZOOM_LEVELS - 1, viewInfos[viewIndex].zoomLevel + 1);
        }

        if (ImGui::IsMouseClicked(2))
        {
            draggingView = viewIndex;
            viewPosOnDragStart[0] = viewInfos[viewIndex].position[0];
            viewPosOnDragStart[1] = viewInfos[viewIndex].position[1];
        }
    }
    if (ImGui::IsMouseDragging(2) && draggingView == viewIndex)
    {
        auto dragDelta = ImGui::GetMouseDragDelta(2);
        viewInfos[viewIndex].position[0] = 
            viewPosOnDragStart[0] - dragDelta.x / ZOOM_LEVELS[viewInfos[viewIndex].zoomLevel];
        viewInfos[viewIndex].position[1] = 
            viewPosOnDragStart[1] - dragDelta.y / ZOOM_LEVELS[viewInfos[viewIndex].zoomLevel];
    }
    if (ImGui::IsMouseReleased(2) && draggingView == viewIndex)
    {
        draggingView = -1;
    }

    viewInfos[viewIndex].type = type;
    viewInfos[viewIndex].x = x;
    viewInfos[viewIndex].y = y;
    viewInfos[viewIndex].w = w;
    viewInfos[viewIndex].h = h;
    viewInfos[viewIndex].index = viewIndex;
    ImGui::GetWindowDrawList()->AddCallback(viewDrawCallback, &viewInfos[viewIndex]);

    ImGui::End();
}

static bool CheckShader(GLuint handle, const char* desc)
{
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if (status == GL_FALSE)
        fprintf(stderr, "ERROR: view.cpp: failed to compile %s!\n", desc);
    if (log_length > 0)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return status == GL_TRUE;
}

static bool CheckProgram(GLuint handle, const char* desc)
{
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if (status == GL_FALSE)
        fprintf(stderr, "ERROR: view.cpp: failed to link %s!\n", desc);
    if (log_length > 0)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return status == GL_TRUE;
}

static GLuint createShaderProgram(const GLchar* vs, const GLchar* ps)
{
    // Create shaders
    const GLchar* vertex_shader_with_version[2] = { "#version 130\n", vs };
    auto vertHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertHandle, 2, vertex_shader_with_version, NULL);
    glCompileShader(vertHandle);
    CheckShader(vertHandle, "vertex shader");

    const GLchar* fragment_shader_with_version[2] = { "#version 130\n", ps };
    auto fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragHandle, 2, fragment_shader_with_version, NULL);
    glCompileShader(fragHandle);
    CheckShader(fragHandle, "fragment shader");

    GLuint program = glCreateProgram();
    glAttachShader(program, vertHandle);
    glAttachShader(program, fragHandle);
    glLinkProgram(program);
    CheckProgram(program, "shader program");

    return program;
}

static void initialize()
{
    initialized = true;

    // 2D Grid
    shader_grid2D.program = createShaderProgram(
        "uniform mat4 WorldMtx;\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec4 Color;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_Color = Color;\n"
        "    vec4 worldPos = WorldMtx * vec4(Position.xy,0,1);\n"
        "    gl_Position = ProjMtx * worldPos;\n"
        "}\n"
        ,
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color;\n"
        "}\n");
    shader_grid2D.uniform_worldMtx = glGetUniformLocation(shader_grid2D.program, "WorldMtx");
    shader_grid2D.uniform_projMtx = glGetUniformLocation(shader_grid2D.program, "ProjMtx");
    shader_grid2D.attrib_position = glGetAttribLocation(shader_grid2D.program, "Position");
    shader_grid2D.attrib_color = glGetAttribLocation(shader_grid2D.program, "Color");
    
    // 3D grid
    shader_grid3D.program = createShaderProgram(
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec4 Color;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n"
        ,
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color;\n"
        "}\n");
    shader_grid3D.uniform_projMtx = glGetUniformLocation(shader_grid3D.program, "ProjMtx");
    shader_grid3D.attrib_position = glGetAttribLocation(shader_grid3D.program, "Position");
    shader_grid3D.attrib_color = glGetAttribLocation(shader_grid3D.program, "Color");

    static const float GRID_COLOR0[4] = { 0.1f, 0.15f, 0.2f, 1 };
    static const float GRID_COLOR1[4] = { 0.2f, 0.3f, 0.4f, 1 };
    static const float GRID_COLOR2[4] = { 0.3f, 0.45f, 0.6f, 1 };
    static const float GRID_COLOR3[4] = { 0.8f, 0.8f, 0.8f, 1 };

    // Grid mesh 0 for the 100x100 km grid
    {
        Grid2DVertex* gridVertices = new Grid2DVertex[GRID_2D_SIZE * 2 * 2];
        for (int i = 0; i < GRID_2D_SIZE; ++i)
        {
            gridVertices[i * 4 + 0].position[0] = (float)i * 1000.0f - 50000.0f;
            gridVertices[i * 4 + 0].position[1] = -50000.0f;
            gridVertices[i * 4 + 1].position[0] = (float)i * 1000.0f - 50000.0f;
            gridVertices[i * 4 + 1].position[1] = 50000.0f;
            gridVertices[i * 4 + 2].position[0] = -50000.0f;
            gridVertices[i * 4 + 2].position[1] = (float)i * 1000.0f - 50000.0f;
            gridVertices[i * 4 + 3].position[0] = 50000.0f;
            gridVertices[i * 4 + 3].position[1] = (float)i * 1000.0f - 50000.0f;
            memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR3, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR3, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR3, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR3, sizeof(float) * 4);
        }

        glGenVertexArrays(1, &mesh_grid0.vao);
        glBindVertexArray(mesh_grid0.vao);

        glGenBuffers(1, &mesh_grid0.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid0.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            GRID_2D_SIZE * 2 * 2 * sizeof(Grid2DVertex),
            (const GLvoid*)gridVertices, GL_STATIC_DRAW);
        delete[] gridVertices;

        glEnableVertexAttribArray(shader_grid2D.attrib_position);
        glEnableVertexAttribArray(shader_grid2D.attrib_color);
        glVertexAttribPointer(shader_grid2D.attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, position));
        glVertexAttribPointer(shader_grid2D.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, color));
    }

    // Grid mesh 1 for zoomed-in grid
    {
        Grid2DVertex* gridVertices = new Grid2DVertex[GRID_2D_SIZE * 2 * 2];
        for (int i = 0; i < GRID_2D_SIZE; ++i)
        {
            gridVertices[i * 4 + 0].position[0] = (float)i - 50.0f;
            gridVertices[i * 4 + 0].position[1] = -50.0f;
            gridVertices[i * 4 + 1].position[0] = (float)i - 50.0f;
            gridVertices[i * 4 + 1].position[1] = 50.0f;
            gridVertices[i * 4 + 2].position[0] = -50.0f;
            gridVertices[i * 4 + 2].position[1] = (float)i - 50.0f;
            gridVertices[i * 4 + 3].position[0] = 50.0f;
            gridVertices[i * 4 + 3].position[1] = (float)i - 50.0f;
            if (i % 10)
            {
                memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR0, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR0, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR0, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR0, sizeof(float) * 4);
            }
            else if (i % 100)
            {
                memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR1, sizeof(float) * 4);
            }
            else
            {
                memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR2, sizeof(float) * 4);
            }
        }

        glGenVertexArrays(1, &mesh_grid1.vao);
        glBindVertexArray(mesh_grid1.vao);

        glGenBuffers(1, &mesh_grid1.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid1.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            GRID_2D_SIZE * 2 * 2 * sizeof(Grid2DVertex),
            (const GLvoid*)gridVertices, GL_STATIC_DRAW);
        delete[] gridVertices;

        glEnableVertexAttribArray(shader_grid2D.attrib_position);
        glEnableVertexAttribArray(shader_grid2D.attrib_color);
        glVertexAttribPointer(shader_grid2D.attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, position));
        glVertexAttribPointer(shader_grid2D.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, color));
    }

    // Grid mesh 2 for zoomed-out grid
    {
        Grid2DVertex* gridVertices = new Grid2DVertex[GRID_2D_SIZE * 2 * 2];
        for (int i = 0; i < GRID_2D_SIZE; ++i)
        {
            gridVertices[i * 4 + 0].position[0] = (float)i * 10.0f - 500.0f;
            gridVertices[i * 4 + 0].position[1] = -500.0f;
            gridVertices[i * 4 + 1].position[0] = (float)i * 10.0f - 500.0f;
            gridVertices[i * 4 + 1].position[1] = 500.0f;
            gridVertices[i * 4 + 2].position[0] = -500.0f;
            gridVertices[i * 4 + 2].position[1] = (float)i * 10.0f - 500.0f;
            gridVertices[i * 4 + 3].position[0] = 500.0f;
            gridVertices[i * 4 + 3].position[1] = (float)i * 10.0f - 500.0f;
            if (i % 10)
            {
                memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR1, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR1, sizeof(float) * 4);
            }
            else
            {
                memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR2, sizeof(float) * 4);
                memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR2, sizeof(float) * 4);
            }
        }

        glGenVertexArrays(1, &mesh_grid2.vao);
        glBindVertexArray(mesh_grid2.vao);

        glGenBuffers(1, &mesh_grid2.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid2.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            GRID_2D_SIZE * 2 * 2 * sizeof(Grid2DVertex),
            (const GLvoid*)gridVertices, GL_STATIC_DRAW);
        delete[] gridVertices;

        glEnableVertexAttribArray(shader_grid2D.attrib_position);
        glEnableVertexAttribArray(shader_grid2D.attrib_color);
        glVertexAttribPointer(shader_grid2D.attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, position));
        glVertexAttribPointer(shader_grid2D.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, color));
    }

    // Grid mesh 3 for meta zoomed-out grid
    {
        Grid2DVertex* gridVertices = new Grid2DVertex[GRID_2D_SIZE * 2 * 2];
        for (int i = 0; i < GRID_2D_SIZE; ++i)
        {
            gridVertices[i * 4 + 0].position[0] = (float)i * 100.0f - 5000.0f;
            gridVertices[i * 4 + 0].position[1] = -5000.0f;
            gridVertices[i * 4 + 1].position[0] = (float)i * 100.0f - 5000.0f;
            gridVertices[i * 4 + 1].position[1] = 5000.0f;
            gridVertices[i * 4 + 2].position[0] = -5000.0f;
            gridVertices[i * 4 + 2].position[1] = (float)i * 100.0f - 5000.0f;
            gridVertices[i * 4 + 3].position[0] = 5000.0f;
            gridVertices[i * 4 + 3].position[1] = (float)i * 100.0f - 5000.0f;
            memcpy(gridVertices[i * 4 + 0].color, GRID_COLOR2, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 1].color, GRID_COLOR2, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 2].color, GRID_COLOR2, sizeof(float) * 4);
            memcpy(gridVertices[i * 4 + 3].color, GRID_COLOR2, sizeof(float) * 4);
        }

        glGenVertexArrays(1, &mesh_grid3.vao);
        glBindVertexArray(mesh_grid3.vao);

        glGenBuffers(1, &mesh_grid3.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid3.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            GRID_2D_SIZE * 2 * 2 * sizeof(Grid2DVertex),
            (const GLvoid*)gridVertices, GL_STATIC_DRAW);
        delete[] gridVertices;

        glEnableVertexAttribArray(shader_grid2D.attrib_position);
        glEnableVertexAttribArray(shader_grid2D.attrib_color);
        glVertexAttribPointer(shader_grid2D.attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, position));
        glVertexAttribPointer(shader_grid2D.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, color));
    }

    //glGenBuffers(1, &mesh_grid.ibo);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_grid.ibo);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridIndices), (const GLvoid*)gridIndices, GL_STATIC_DRAW);
}

#define TORAD 0.01745329251994329576923690768489f

static void createViewMatrix(const float position[3], float angleX, float angleZ, float out[4][4])
{
    // Normalized direction
    float R2[3] = {
        -std::sinf(angleZ * TORAD) * std::cosf(angleX * TORAD),
        -std::cosf(angleZ * TORAD) * std::cosf(angleX * TORAD),
        std::sinf(angleX * TORAD)
    };

    float R0[3] = {
        -1 * R2[1],
        1 * R2[0],
        0
    };
    float len = std::sqrtf(R0[0] * R0[0] + R0[1] * R0[1]);
    R0[0] /= len;
    R0[1] /= len;

    float R1[3] = {
        R2[1] * R0[2] - R2[2] * R0[1],
        R2[2] * R0[0] - R2[0] * R0[2],
        R2[0] * R0[1] - R2[1] * R0[0]
    };

    auto D0 = R0[0] * -position[0] + R0[1] * -position[1] + R0[2] * -position[2];
    auto D1 = R1[0] * -position[0] + R1[1] * -position[1] + R1[2] * -position[2];
    auto D2 = R2[0] * -position[0] + R2[1] * -position[1] + R2[2] * -position[2];

    out[0][0] = R0[0];
    out[0][1] = R1[0];
    out[0][2] = R2[0];
    out[0][3] = 0.0f;

    out[1][0] = R0[1];
    out[1][1] = R1[1];
    out[1][2] = R2[1];
    out[1][3] = 0.0f;

    out[2][0] = R0[2];
    out[2][1] = R1[2];
    out[2][2] = R2[2];
    out[2][3] = 0.0f;

    out[3][0] = D0;
    out[3][1] = D1;
    out[3][2] = D2;
    out[3][3] = 1.0f;
}

static void createPerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane, float out[4][4])
{
    float CosFov = std::cos(0.5f * fov * TORAD);
    float SinFov = std::sin(0.5f * fov * TORAD);

    float Height = CosFov / SinFov;
    float Width = Height / aspectRatio;
    float fRange = farPlane / (nearPlane - farPlane);

    out[0][0] = Width;
    out[0][1] = 0.0f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;

    out[1][0] = 0.0f;
    out[1][1] = Height;
    out[1][2] = 0.0f;
    out[1][3] = 0.0f;

    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[2][2] = fRange;
    out[2][3] = -1.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = fRange * nearPlane;
    out[3][3] = 0.0f;
}

static void mulMatrix(float a[4][4], float b[4][4], float out[4][4])
{
    memcpy(out, a, sizeof(out) * 16);
    // Cache the invariants in registers
    float x = out[0][0];
    float y = out[0][1];
    float z = out[0][2];
    float w = out[0][3];
    // Perform the operation on the first row
    out[0][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[0][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[0][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[0][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    // Repeat for all the other rows
    x = out[1][0];
    y = out[1][1];
    z = out[1][2];
    w = out[1][3];
    out[1][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[1][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[1][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[1][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    x = out[2][0];
    y = out[2][1];
    z = out[2][2];
    w = out[2][3];
    out[2][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[2][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[2][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[2][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    x = out[3][0];
    y = out[3][1];
    z = out[3][2];
    w = out[3][3];
    out[3][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[3][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[3][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[3][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
}

static void viewDrawCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    auto pViewInfo = (ViewInfo*)cmd->UserCallbackData;

    GLStates glStates;
    saveGLStates(&glStates);

    // Lazy init
    if (!initialized) initialize();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    glViewport((GLint)cmd->ClipRect.x, height - (GLint)cmd->ClipRect.w,
        (GLsizei)(cmd->ClipRect.z - cmd->ClipRect.x),
        (GLsizei)(cmd->ClipRect.w - cmd->ClipRect.y));

    float W = cmd->ClipRect.z - cmd->ClipRect.x;
    float H = cmd->ClipRect.w - cmd->ClipRect.y;

    if (pViewInfo->type == ViewType::Perspective)
    {
        float viewMat[4][4];
        float projMat[4][4];
        float viewProjMat[4][4];

        createViewMatrix(pViewInfo->position, pViewInfo->angleX, pViewInfo->angleZ, viewMat);
        createPerspectiveFieldOfView(90, W / H, 0.1f, 1000.0f, projMat);
        mulMatrix(viewMat, projMat, viewProjMat);

        glUseProgram(shader_grid3D.program);
        glUniformMatrix4fv(shader_grid3D.uniform_projMtx, 1, GL_FALSE, &viewProjMat[0][0]);

        // Zoomed-in grid
        glBindVertexArray(mesh_grid1.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid1.vbo);
        glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);

        //// Absolute 0 guides
        //glBindVertexArray(mesh_grid0.vao);
        //glBindBuffer(GL_ARRAY_BUFFER, mesh_grid0.vbo);
        //glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
    }
    else
    {
        // 2D drawing
        float L = 0;
        float R = cmd->ClipRect.z - cmd->ClipRect.x;
        float T = 0;
        float B = cmd->ClipRect.w - cmd->ClipRect.y;
        const float ortho_projection[4][4] =
        {
            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f }
        };

        float Zoom = ZOOM_LEVELS[pViewInfo->zoomLevel];
        float X = pViewInfo->position[0];
        float Y = pViewInfo->position[1];
        const float world_matrix[4][4] =
        {
            { Zoom,  0.0f,  0.0f,  0.0f },
            { 0.0f,  Zoom,  0.0f,  0.0f },
            { 0.0f,  0.0f,  Zoom,  0.0f },
            { W/2.0f-X*Zoom,  H/2.0f-Y*Zoom,  0.0f,  1.0f }
        };

        glUseProgram(shader_grid2D.program);
        glUniformMatrix4fv(shader_grid2D.uniform_worldMtx, 1, GL_FALSE, &world_matrix[0][0]);
        glUniformMatrix4fv(shader_grid2D.uniform_projMtx, 1, GL_FALSE, &ortho_projection[0][0]);

        if (pViewInfo->zoomLevel >= 13)
        {
            // Zoomed-in
            glBindVertexArray(mesh_grid1.vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh_grid1.vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }
        else if (pViewInfo->zoomLevel >= 7)
        {
            // Zoomed-out
            glBindVertexArray(mesh_grid2.vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh_grid2.vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }
        else if (pViewInfo->zoomLevel >= 1)
        {
            // Meta Zoomed-out
            glBindVertexArray(mesh_grid3.vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh_grid3.vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }

        // Absolute 0 guides
        glBindVertexArray(mesh_grid0.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_grid0.vbo);
        glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);

        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_grid1.ibo);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void*)(uintptr_t)(0));
    }

    restoreGLStates(&glStates);
}
