#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "fluid.hpp"
#include "render.hpp"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow(800, 800, "Fluid Simulation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!initGL()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return -1;
    }

    initFluid();
    setupInputCallbacks(window);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;

        float adjustedDt = std::min(dt, float(frameTime * 5.0));
        updateFluid(adjustedDt);
        updateVBO();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
        checkGLError("main loop");
    }

    cleanupGL();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}