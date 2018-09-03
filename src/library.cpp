#include "library.h"
#include "globals.h"
#include "rendering.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <imgui.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tinyfiledialogs.h>

#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

struct MeshVertex
{
    float position[3];
    float normal[3];
    float color[4];
    float uv[2];
};

struct Thumbnail
{
    std::string name;
    GLuint thumbnail;
};

MeshShader meshShader;

static bool initialized = false;
static uint64_t nextId = 1;
static std::unordered_map<std::string, GLuint> textures;
static std::unordered_map<uint64_t, Model> models;
static std::vector<Thumbnail> thumbnails;
static aiPropertyStore* propertyStore;

Model library_getModel(uint64_t id)
{
    return models[id];
}

static void initialize()
{
    propertyStore = aiCreatePropertyStore();

    meshShader.program = createShaderProgram(
        "uniform mat4 WorldMtx;\n"
        "uniform mat4 ProjMtx;\n"
        "in vec3 Position;\n"
        "in vec3 Normal;\n"
        "in vec4 Color;\n"
        "in vec2 TexCoord;\n"
        "out vec3 Frag_Normal;\n"
        "out vec4 Frag_Color;\n"
        "out vec2 Frag_TexCoord;\n"
        "void main()\n"
        "{\n"
        "    Frag_Normal = normalize((WorldMtx * vec4(Normal.xyz, 1)).xyz);\n"
        "    Frag_Color = Color;\n"
        "    Frag_TexCoord = TexCoord;\n"
        "    vec4 worldPos = WorldMtx * vec4(Position.xyz,1);\n"
        "    gl_Position = ProjMtx * worldPos;\n"
        "}\n"
        ,
        "in vec3 Frag_Normal;\n"
        "in vec4 Frag_Color;\n"
        "in vec2 Frag_TexCoord;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * mix(0.7, 1.0, Frag_Normal.z * 0.5 + 0.5) * mix(0.8, 1.0, abs(Frag_Normal.x));\n"
        "}\n");
    glUseProgram(meshShader.program);
    meshShader.uniform_worldMtx = glGetUniformLocation(meshShader.program, "WorldMtx");
    meshShader.uniform_projMtx = glGetUniformLocation(meshShader.program, "ProjMtx");
    meshShader.attrib_position = glGetAttribLocation(meshShader.program, "Position");
    meshShader.attrib_normal = glGetAttribLocation(meshShader.program, "Normal");
    meshShader.attrib_color = glGetAttribLocation(meshShader.program, "Color");
    meshShader.attrib_texCoord = glGetAttribLocation(meshShader.program, "TexCoord");

    initialized = true;
}

static GLuint loadTexture(const std::string& filename)
{
    auto path = document.filename.substr(0, document.filename.find_last_of("/\\") + 1) + filename;

    auto it = textures.find(path);
    if (it != textures.end()) return it->second;

    int w, h, bpp;
    auto imageData = stbi_load(path.c_str(), &w, &h, &bpp, 4);
    if (!imageData)
    {
        // Don't die, he will see while texture
        // TODO: use checker board
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    textures[path] = texture;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(imageData);

    return texture;
}

static Model loadModel(const std::string& filename, float scale)
{
    auto path = document.filename.substr(0, document.filename.find_last_of("/\\") + 1) + filename;

    //aiSetImportPropertyFloat(propertyStore, AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, scale);
    const aiScene* pScene = aiImportFile(path.c_str(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_PreTransformVertices |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType/* |
        aiProcess_GlobalScale*/);

    if (!pScene)
    {
        tinyfd_messageBox("Loading Model", ("Failed to open model:\n" + filename + "\n" + aiGetErrorString()).c_str(), "ok", "error", 0);
        return { 0 };
    }

    Model model;
    model.count = (int)pScene->mNumMeshes;
    model.meshes = new Mesh[model.count];

    for (int i = 0; i < model.count; ++i)
    {
        auto pMesh = model.meshes + i;
        auto pAssMesh = pScene->mMeshes[i];

        if (pAssMesh->mNumVertices == 0)
        {
            tinyfd_messageBox("Loading Model", ("A mesh has no vertices:\n" + filename).c_str(), "ok", "error", 0);
            return { 0 };
        }

        // Load from the file
        MeshVertex* vertices = new MeshVertex[pAssMesh->mNumVertices];
        for (int i = 0; i < (int)pAssMesh->mNumVertices; ++i)
        {
            auto pVertex = vertices + i;
            pVertex->position[0] = pAssMesh->mVertices[i].x * scale;
            pVertex->position[1] = -pAssMesh->mVertices[i].z * scale;
            pVertex->position[2] = pAssMesh->mVertices[i].y * scale;
        }
        for (int i = 0; i < (int)pAssMesh->mNumVertices; ++i)
        {
            auto pVertex = vertices + i;
            memcpy(pVertex->normal, pAssMesh->mNormals + i, sizeof(float) * 3);
        }
        if (pAssMesh->HasVertexColors(0))
        {
            auto colorCnt = pAssMesh->GetNumColorChannels();
            for (int i = 0; i < (int)pAssMesh->mNumVertices; ++i)
            {
                auto pVertex = vertices + i;
                memcpy(pVertex->color, pAssMesh->mColors[0] + i * colorCnt, sizeof(float) * 4);
            }
        }
        else
        {
            for (int i = 0; i < (int)pAssMesh->mNumVertices; ++i)
            {
                auto pVertex = vertices + i;
                pVertex->color[0] = 1; pVertex->color[1] = 1; pVertex->color[2] = 1; pVertex->color[3] = 1;
            }
        }
        if (pAssMesh->HasTextureCoords(0))
        {
            auto uvCnt = pAssMesh->GetNumUVChannels();
            for (int i = 0; i < (int)pAssMesh->mNumVertices; ++i)
            {
                auto pVertex = vertices + i;
                memcpy(pVertex->uv, pAssMesh->mTextureCoords[0] + i * uvCnt, sizeof(float) * 2);
            }
        }

        glGenVertexArrays(1, &pMesh->vao);
        glBindVertexArray(pMesh->vao);

        glGenBuffers(1, &pMesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, pAssMesh->mNumVertices * sizeof(MeshVertex), (const GLvoid*)vertices, GL_STATIC_DRAW);
        delete[] vertices;

        glEnableVertexAttribArray(meshShader.attrib_position);
        glEnableVertexAttribArray(meshShader.attrib_normal);
        glEnableVertexAttribArray(meshShader.attrib_color);
        glEnableVertexAttribArray(meshShader.attrib_texCoord);
        glVertexAttribPointer(meshShader.attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)IM_OFFSETOF(MeshVertex, position));
        glVertexAttribPointer(meshShader.attrib_normal, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)IM_OFFSETOF(MeshVertex, normal));
        glVertexAttribPointer(meshShader.attrib_color, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)IM_OFFSETOF(MeshVertex, color));
        glVertexAttribPointer(meshShader.attrib_texCoord, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (GLvoid*)IM_OFFSETOF(MeshVertex, uv));

        // Load faces
        pMesh->elementCount = (GLsizei)pAssMesh->mNumFaces * 3;
        if (pAssMesh->mNumFaces * 3 > std::numeric_limits<uint16_t>::max())
        {
            uint32_t* indices = new uint32_t[pMesh->elementCount];
            for (int i = 0; i < (int)pAssMesh->mNumFaces; ++i)
            {
                indices[i * 3 + 0] = (uint32_t)pAssMesh->mFaces[i].mIndices[0];
                indices[i * 3 + 1] = (uint32_t)pAssMesh->mFaces[i].mIndices[1];
                indices[i * 3 + 2] = (uint32_t)pAssMesh->mFaces[i].mIndices[2];
            }
            glGenBuffers(1, &pMesh->ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * pMesh->elementCount, (const GLvoid*)indices, GL_STATIC_DRAW);
            delete[] indices;
            pMesh->elementType = GL_UNSIGNED_INT;
        }
        else
        {
            uint16_t* indices = new uint16_t[pMesh->elementCount];
            for (int i = 0; i < (int)pAssMesh->mNumFaces; ++i)
            {
                indices[i * 3 + 0] = (uint16_t)pAssMesh->mFaces[i].mIndices[0];
                indices[i * 3 + 1] = (uint16_t)pAssMesh->mFaces[i].mIndices[1];
                indices[i * 3 + 2] = (uint16_t)pAssMesh->mFaces[i].mIndices[2];
            }
            glGenBuffers(1, &pMesh->ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * pMesh->elementCount, (const GLvoid*)indices, GL_STATIC_DRAW);
            delete[] indices;
            pMesh->elementType = GL_UNSIGNED_SHORT;
        }
    }

    aiReleaseImport(pScene);

    return model;
}

void library_load()
{
    // Lazy init
    if (!initialized) initialize();

    nextId = 1;

    for (const auto& kv : textures)
    {
        glDeleteTextures(1, &kv.second);
    }
    textures.clear();

    //for (const auto& kv : models)
    //{
    //    if (kv.second.thumbnail) glDeleteTextures(1, &kv.second.thumbnail);
    //}
    //models.clear();

    const auto& jsonLibrary = document.json["library"];
    for (int i = 0; i < (int)jsonLibrary.size(); ++i)
    {
        const auto& jsonModel = jsonLibrary[i];
        //Model model;
        //model.id = jsonModel["id"].asUInt64();
        //model.diffuse = loadTexture(jsonModel["diffuse"].asString());
        //model.thumbnail = 0;

        auto id = jsonModel["id"].asUInt64();

        Thumbnail thumbnail;
        thumbnail.name = jsonModel["name"].asString();
        thumbnail.thumbnail = loadTexture(jsonModel["diffuse"].asString());
        thumbnails.push_back(thumbnail);

        models[id] = loadModel(jsonModel["filename"].asString(), jsonModel["scale"].asFloat());

        nextId = std::max(nextId, id + 1);
    }
}

void library_updateGUI()
{
    if (!isRightPanelVisible) return;

    auto totalH = (float)height - 52;
    auto hTop = totalH * 1.0f / 3.0f;
    auto hBottom = totalH * 2.0f / 3.0f;

    ImGui::SetNextWindowPos({ (float)width - PANEL_WIDTH, 52 + hTop });
    ImGui::SetNextWindowSize({ PANEL_WIDTH, hBottom });
    ImGui::Begin("Library", 0,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);
    ImGui::Columns(3, 0, false);
    for (const auto& thumbnail : thumbnails)
    {
        ImGui::Image((ImTextureID)thumbnail.thumbnail, ImVec2(64, 64));
        ImGui::Text(thumbnail.name.c_str());
        ImGui::NextColumn();
    }
    ImGui::End();
}
