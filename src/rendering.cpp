#include "rendering.h"

#include <imgui.h>
#include <tinyfiledialogs.h>

#include <stdio.h>
#include <string>

void saveGLStates(GLStates* pStates)
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

void restoreGLStates(GLStates* pStates)
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
        tinyfd_messageBox("Check Shader", (std::string("Failed to build shader") + buf.begin()).c_str(), "ok", "error", 0);
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
        tinyfd_messageBox("Check Program", (std::string("Failed to build shader") + buf.begin()).c_str(), "ok", "error", 0);
    }
    return status == GL_TRUE;
}

GLuint createShaderProgram(const GLchar* vs, const GLchar* ps)
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
