#ifndef RENDER_HPP
#define RENDER_HPP
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>   

extern GLuint vao, vbo, ebo, shaderProgram;
extern GLint posAttrib, colorAttrib;

bool initGL();
void cleanupGL();
void updateVBO();
void render();
void setupInputCallbacks(GLFWwindow* window);
void checkGLError(const std::string& place);

#endif