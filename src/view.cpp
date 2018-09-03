#include "view.h"
#include "globals.h"
#include "math_helper.h"
#include "rendering.h"
#include "library.h"

#include <imgui.h>
#include <stdio.h>
#include <cinttypes>
#include <SDL.h>

// Defs
#define GRID_2D_SIZE 101
#define GRIDMESH_MAX 4

// Types
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

struct Grid2DVertex
{
    float position[2];
    float color[4];
};

// Private vars
static GridMesh gridMeshes[GRIDMESH_MAX];
static int draggingView = -1;
static bool initialized = false;
static float viewPosOnDragStart[2] = { 0, 0 };
static int dragMouseX, dragMouseY;

// Public vars
const char* VIEW_TYPE_TO_NAME[] = {
    "perspective",
    "top",
    "left",
    "front",
    "bottom",
    "right",
    "back"
};
ViewInfo viewInfos[MAX_VIEWS] = {
    { { -2, -2, 2 }, 15, -30, 45 },
    { { 0, 0, 0 }, 18, 0, 0 },
    { { 0, 0, 0 }, 18, 0, 0 },
    { { 0, 0, 0 }, 18, 0, 0 }
};

// Prototypes
static void viewDrawCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);

// Functions
void view_updateGUI(ViewType type, ViewLayout layout, int viewIndex)
{
    auto x = 0.0f;
    auto y = 52.0f;
    auto w = (float)width;
    auto h = (float)height - y;
    auto pView = viewInfos + viewIndex;

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
        if (pView->type != ViewType::Perspective)
        {
            if (io.MouseWheel < -0.1f)
            {
                pView->zoomLevel =
                    std::max(0, pView->zoomLevel - 1);

                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["zoom"] = pView->zoomLevel;
                document.dirty = true;
            }
            else if (io.MouseWheel > 0.1f)
            {
                pView->zoomLevel =
                    std::min(MAX_ZOOM_LEVELS - 1, pView->zoomLevel + 1);

                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["zoom"] = pView->zoomLevel;
                document.dirty = true;
            }
        }

        if (ImGui::IsMouseClicked(2))
        {
            draggingView = viewIndex;
            if (pView->type == ViewType::Perspective)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);
                SDL_GetMouseState(&dragMouseX, &dragMouseY);
                updateEachFrame = true;
            }
            else
            {
                viewPosOnDragStart[0] = pView->position[0];
                viewPosOnDragStart[1] = pView->position[1];
            }
        }
    }

    if (ImGui::IsMouseReleased(2) && draggingView == viewIndex)
    {
        draggingView = -1;
        if (pView->type == ViewType::Perspective)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            updateEachFrame = false;
        }
    }

    if (draggingView == viewIndex)
    {
        if (viewInfos[viewIndex].type == ViewType::Perspective)
        {
            int mouseDeltaX, mouseDeltaY;
            SDL_GetMouseState(&mouseDeltaX, &mouseDeltaY);
            mouseDeltaX -= dragMouseX;
            mouseDeltaY -= dragMouseY;

            bool dirty = mouseDeltaX || mouseDeltaY;

            pView->angleZ += (float)mouseDeltaX * 0.3f;
            pView->angleZ = std::fmodf(viewInfos[viewIndex].angleZ, 360.0f);
            pView->angleX -= (float)mouseDeltaY * 0.3f;
            pView->angleX = std::max(-89.0f, std::min(89.0f, pView->angleX));

            float front[3] = {
                std::sinf(pView->angleZ * TORAD) * std::cosf(pView->angleX * TORAD),
                std::cosf(pView->angleZ * TORAD) * std::cosf(pView->angleX * TORAD),
                std::sinf(pView->angleX * TORAD)
            };
            float right[2] = {
                std::cosf(pView->angleZ * TORAD),
                -std::sinf(pView->angleZ * TORAD)
            };

            ImGuiIO& io = ImGui::GetIO();
            if (io.KeysDown[SDL_SCANCODE_W])
            {
                pView->position[0] += front[0] * 5.0f * io.DeltaTime;
                pView->position[1] += front[1] * 5.0f * io.DeltaTime;
                pView->position[2] += front[2] * 5.0f * io.DeltaTime;
                dirty = true;
            }
            if (io.KeysDown[SDL_SCANCODE_S])
            {
                pView->position[0] -= front[0] * 5.0f * io.DeltaTime;
                pView->position[1] -= front[1] * 5.0f * io.DeltaTime;
                pView->position[2] -= front[2] * 5.0f * io.DeltaTime;
                dirty = true;
            }
            if (io.KeysDown[SDL_SCANCODE_D])
            {
                pView->position[0] += right[0] * 5.0f * io.DeltaTime;
                pView->position[1] += right[1] * 5.0f * io.DeltaTime;
                dirty = true;
            }
            if (io.KeysDown[SDL_SCANCODE_A])
            {
                pView->position[0] -= right[0] * 5.0f * io.DeltaTime;
                pView->position[1] -= right[1] * 5.0f * io.DeltaTime;
                dirty = true;
            }
            if (io.KeysDown[SDL_SCANCODE_E])
            {
                pView->position[2] += 5.0f * io.DeltaTime;
                dirty = true;
            }
            if (io.KeysDown[SDL_SCANCODE_Q])
            {
                pView->position[2] -= 5.0f * io.DeltaTime;
                dirty = true;
            }

            if (dirty)
            {
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["position"]["x"] = pView->position[0];
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["position"]["y"] = pView->position[1];
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["position"]["z"] = pView->position[2];

                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["angleX"] = pView->angleX;
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["angleZ"] = pView->angleZ;

                document.dirty = true;
            }

            SDL_WarpMouseInWindow(NULL, dragMouseX, dragMouseY);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        }
        else
        {
            auto dragDelta = ImGui::GetMouseDragDelta(2);
            bool dirty = dragDelta.x || dragDelta.y;

            pView->position[0] = viewPosOnDragStart[0] - dragDelta.x / ZOOM_LEVELS[pView->zoomLevel];
            pView->position[1] = viewPosOnDragStart[1] - dragDelta.y / ZOOM_LEVELS[pView->zoomLevel];

            if (dirty)
            {
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["position"]["x"] = pView->position[0];
                document.json["editor"]["views"][VIEW_TYPE_TO_NAME[viewIndex]]["position"]["y"] = pView->position[1];

                document.dirty = true;
            }
        }
    }

    pView->type = type;
    pView->x = x;
    pView->y = y;
    pView->w = w;
    pView->h = h;
    pView->index = viewIndex;

    ImGui::GetWindowDrawList()->AddCallback(viewDrawCallback, pView);
    ImGui::Text("%0.2f, %0.2f, %i", pView->position[0], pView->position[1], pView->zoomLevel);

    ImGui::End();
}

void view_load()
{
    for (int i = 0; i < MAX_VIEWS; ++i)
    {
        auto pView = viewInfos + i;

        pView->position[0] = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["position"]["x"].asFloat();
        pView->position[1] = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["position"]["y"].asFloat();
        pView->position[2] = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["position"]["z"].asFloat();
        pView->angleX = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["angleX"].asFloat();
        pView->angleZ = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["angleZ"].asFloat();
        pView->zoomLevel = document.json["editor"]["views"][VIEW_TYPE_TO_NAME[i]]["zoom"].asInt();
    }
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
    glUseProgram(shader_grid2D.program);
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
    glUseProgram(shader_grid3D.program);
    shader_grid3D.uniform_projMtx = glGetUniformLocation(shader_grid3D.program, "ProjMtx");
    shader_grid3D.attrib_position = glGetAttribLocation(shader_grid3D.program, "Position");
    shader_grid3D.attrib_color = glGetAttribLocation(shader_grid3D.program, "Color");

    static const float GRID_COLORS[][4] = {
        { 0.1f, 0.15f, 0.2f, 1 },
        { 0.2f, 0.3f, 0.4f, 1 },
        { 0.3f, 0.45f, 0.6f, 1 },
        { 0.8f, 0.8f, 0.8f, 1 }
    };
    float scale = 1.0f;
    for (int g = 0; g < GRIDMESH_MAX; ++g)
    {
        auto pGridMesh = gridMeshes + g;

        Grid2DVertex* gridVertices = new Grid2DVertex[GRID_2D_SIZE * 2 * 2];
        Grid2DVertex* pVertex = gridVertices;

        auto pColor1 = GRID_COLORS[g];
        auto pColor2 = GRID_COLORS[std::min(3, g + 1)];

        for (int i = 0; i < GRID_2D_SIZE; ++i)
        {
            if (!(i % 10)) continue;
            pVertex[0].position[0] = (float)i * scale - 50.0f * scale;
            pVertex[0].position[1] = -50.0f * scale;
            pVertex[1].position[0] = (float)i * scale - 50.0f * scale;
            pVertex[1].position[1] = 50.0f * scale;
            pVertex[2].position[0] = -50.0f * scale;
            pVertex[2].position[1] = (float)i * scale - 50.0f * scale;
            pVertex[3].position[0] = 50.0f * scale;
            pVertex[3].position[1] = (float)i * scale - 50.0f * scale;
            memcpy(pVertex[0].color, pColor1, sizeof(float) * 4);
            memcpy(pVertex[1].color, pColor1, sizeof(float) * 4);
            memcpy(pVertex[2].color, pColor1, sizeof(float) * 4);
            memcpy(pVertex[3].color, pColor1, sizeof(float) * 4);
            pVertex += 4;
        }
        for (int i = 0; i < GRID_2D_SIZE; i += 10)
        {
            pVertex[0].position[0] = (float)i * scale - 50.0f * scale;
            pVertex[0].position[1] = -50.0f * scale;
            pVertex[1].position[0] = (float)i * scale - 50.0f * scale;
            pVertex[1].position[1] = 50.0f * scale;
            pVertex[2].position[0] = -50.0f * scale;
            pVertex[2].position[1] = (float)i * scale - 50.0f * scale;
            pVertex[3].position[0] = 50.0f * scale;
            pVertex[3].position[1] = (float)i * scale - 50.0f * scale;
            memcpy(pVertex[0].color, pColor2, sizeof(float) * 4);
            memcpy(pVertex[1].color, pColor2, sizeof(float) * 4);
            memcpy(pVertex[2].color, pColor2, sizeof(float) * 4);
            memcpy(pVertex[3].color, pColor2, sizeof(float) * 4);
            pVertex += 4;
        }

        glGenVertexArrays(1, &pGridMesh->vao);
        glBindVertexArray(pGridMesh->vao);

        glGenBuffers(1, &pGridMesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, pGridMesh->vbo);
        glBufferData(GL_ARRAY_BUFFER,
            GRID_2D_SIZE * 2 * 2 * sizeof(Grid2DVertex),
            (const GLvoid*)gridVertices, GL_STATIC_DRAW);
        delete[] gridVertices;

        glEnableVertexAttribArray(shader_grid2D.attrib_position);
        glEnableVertexAttribArray(shader_grid2D.attrib_color);
        glVertexAttribPointer(shader_grid2D.attrib_position, 2, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, position));
        glVertexAttribPointer(shader_grid2D.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(Grid2DVertex), (GLvoid*)IM_OFFSETOF(Grid2DVertex, color));

        scale *= 10.0f;
    }

    //glGenBuffers(1, &mesh_grid.ibo);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_grid.ibo);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridIndices), (const GLvoid*)gridIndices, GL_STATIC_DRAW);
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

        // Draw 3D models
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        glUseProgram(meshShader.program);
        glUniformMatrix4fv(meshShader.uniform_projMtx, 1, GL_FALSE, &viewProjMat[0][0]);

        for (int i = 0; i < (int)document.json["map"].size(); ++i)
        {
            auto jsonObj = document.json["map"][i];
            auto model = library_getModel(jsonObj["modelId"].asUInt64());

            const float world_matrix[4][4] = {
                { 1, 0, 0, 0 },
                { 0, 1, 0, 0 },
                { 0, 0, 1, 0 },
                { jsonObj["position"]["x"].asFloat(), jsonObj["position"]["y"].asFloat(), jsonObj["position"]["z"].asFloat(), 1 }
            };
            glUniformMatrix4fv(meshShader.uniform_worldMtx, 1, GL_FALSE, &world_matrix[0][0]);

            for (int j = 0; j < model.count; ++j)
            {
                auto pMesh = model.meshes + j;
                glBindVertexArray(pMesh->vao);
                glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
                glDrawElements(GL_TRIANGLES, pMesh->elementCount, pMesh->elementType, (const void*)(uintptr_t)(0));
            }
        }

        // Draw Grid
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        glUseProgram(shader_grid3D.program);
        glUniformMatrix4fv(shader_grid3D.uniform_projMtx, 1, GL_FALSE, &viewProjMat[0][0]);

        // Zoomed-in grid
        glBindVertexArray(gridMeshes[0].vao);
        glBindBuffer(GL_ARRAY_BUFFER, gridMeshes[0].vbo);
        glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
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
            glBindVertexArray(gridMeshes[0].vao);
            glBindBuffer(GL_ARRAY_BUFFER, gridMeshes[0].vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }
        else if (pViewInfo->zoomLevel >= 7)
        {
            // Zoomed-out
            glBindVertexArray(gridMeshes[1].vao);
            glBindBuffer(GL_ARRAY_BUFFER, gridMeshes[1].vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }
        else if (pViewInfo->zoomLevel >= 1)
        {
            // Meta Zoomed-out
            glBindVertexArray(gridMeshes[2].vao);
            glBindBuffer(GL_ARRAY_BUFFER, gridMeshes[2].vbo);
            glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
        }

        // Absolute 0 guides
        glBindVertexArray(gridMeshes[3].vao);
        glBindBuffer(GL_ARRAY_BUFFER, gridMeshes[3].vbo);
        glDrawArrays(GL_LINES, 0, GRID_2D_SIZE * 2 * 2);
    }

    restoreGLStates(&glStates);
}
