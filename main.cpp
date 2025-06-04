#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h> // Use GLES3 for WebGL 2.0
#include <iostream>
#include "fluid.hpp"
#include "render.hpp"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

void main_loop() {
    static double lastTime = emscripten_get_now() / 1000.0;
    double currentTime = emscripten_get_now() / 1000.0;
    double frameTime = currentTime - lastTime;
    lastTime = currentTime;

    float adjustedDt = std::min(dt, float(frameTime * 5.0));
    updateFluid(adjustedDt);
    updateVBO();
    render();

    checkGLError("main loop");
}

int main() {
    // Initialize WebGL context
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2; // WebGL 2.0
    attrs.minorVersion = 0;
    context = emscripten_webgl_create_context("#canvas", &attrs);
    if (context <= 0) {
        printf("Failed to create WebGL context\n");
        return -1;
    }
    emscripten_webgl_make_context_current(context);

    // Initialize OpenGL
    if (!initGL()) {
        printf("Failed to initialize OpenGL\n");
        return -1;
    }

    // Initialize fluid simulation
    initFluid();

    // Set up input callbacks (handled in render.cpp via Emscripten APIs)
    setupInputCallbacks();

    // Set up the main loop
    emscripten_set_main_loop(main_loop, 0, 1);

    return 0;
}