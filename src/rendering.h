#ifndef RENDERING_H_INCLUDED
#define RENDERING_H_INCLUDED

#include <GL/gl3w.h>

struct GLStates
{
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

void saveGLStates(GLStates* pStates);
void restoreGLStates(GLStates* pStates);
GLuint createShaderProgram(const GLchar* vs, const GLchar* ps);

#endif
