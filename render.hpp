#ifndef RENDER_HPP
#define RENDER_HPP
#include <GLES3/gl3.h>
#include <string>   

extern GLuint vao, vbo, ebo, shaderProgram;
extern GLint posAttrib, colorAttrib;

bool initGL();
void cleanupGL();
void updateVBO();
void render();
void setupInputCallbacks();
void checkGLError(const std::string& place);

#endif