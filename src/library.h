#ifndef LIBRARY_H_INCLUDED
#define LIBRARY_H_INCLUDED

#include <GL/gl3w.h>

#include <cinttypes>

struct Material
{
    GLuint diffuse;
};

struct MeshShader
{
    GLuint program = 0;
    GLint uniform_texture = 0;
    GLint uniform_worldMtx = 0;
    GLint uniform_projMtx = 0;
    GLint attrib_position = 0;
    GLint attrib_normal = 0;
    GLint attrib_color = 0;
    GLint attrib_texCoord = 0;
};

struct Mesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLsizei elementCount = 0;
    GLuint elementType = GL_UNSIGNED_SHORT;
    Material* pMaterial;
};

struct Model
{
    int meshCount;
    Mesh* meshes;
    int materialCount;
    Material* materials;
};

void library_load();
void library_updateGUI();
Model library_getModel(uint64_t id);

extern MeshShader meshShader;

#endif
