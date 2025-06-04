#include "render.hpp"
#include "fluid.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <vector>
#include <algorithm>
#include <cstdio>

// OpenGL variables
GLuint vao, vbo, ebo, shaderProgram;
GLint posAttrib, colorAttrib;

bool isDragging = false;
double startX, startY;

// Shader sources (modified for WebGL 2.0)
const char* vertexShaderSource = R"(
    #version 300 es
    precision highp float;
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec3 color;
    out vec3 fragColor;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        fragColor = color;
    }
)";

const char* fragmentShaderSource = R"(
    #version 300 es
    precision highp float;
    in vec3 fragColor;
    out vec4 outColor;
    void main() {
        outColor = vec4(fragColor, 1.0);
    }
)";

bool compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
        return false;
    }
    return true;
}

bool initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vertexShader, vertexShaderSource)) {
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fragmentShader, fragmentShaderSource)) {
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram/shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Shader program linking error: %s\n", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    return true;
}

void cleanupGL() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
}

void updateVBO() {
    // Same as original, no changes needed for WebGL 2.0
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float scale = 2.0f / N;
    
    vertices.reserve(N * N * 5);
    indices.reserve(N * N * 6);
    
    unsigned int vertexCount = 0;
    
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            float x = (i - 0.5f) * scale - 1.0f;
            float y = (j - 0.5f) * scale - 1.0f;
            float d = dens[IX(i,j)];
            d = std::min(std::max(d, 0.0f), 1.0f);

            vertices.push_back(x); vertices.push_back(y);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            vertices.push_back(x + scale); vertices.push_back(y);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            vertices.push_back(x + scale); vertices.push_back(y + scale);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            vertices.push_back(x); vertices.push_back(y + scale);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            indices.push_back(vertexCount);
            indices.push_back(vertexCount + 1);
            indices.push_back(vertexCount + 2);
            indices.push_back(vertexCount);
            indices.push_back(vertexCount + 2);
            indices.push_back(vertexCount + 3);
            
            vertexCount += 4;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_DYNAMIC_DRAW);
                 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    
    posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
                         5 * sizeof(float), (void*)0);

    colorAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colorAttrib);
    glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE,
                         5 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    checkGLError("updateVBO");
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, N * N * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    checkGLError("render");
}

bool isValidGridCell(int i, int j) {
    return (i >= 1 && i <= N && j >= 1 && j <= N);
}

EMSCRIPTEN_KEEPALIVE
void mouse_button_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    if (mouseEvent->button == 0) { // Left button
        if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
            isDragging = true;
            startX = mouseEvent->targetX;
            startY = mouseEvent->targetY;
        } else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
            isDragging = false;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void cursor_pos_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    static double lastX = mouseEvent->targetX, lastY = mouseEvent->targetY;
    static bool firstMove = true;

    if (firstMove) {
        firstMove = false;
        lastX = mouseEvent->targetX;
        lastY = mouseEvent->targetY;
        return;
    }

    int width, height;
    emscripten_get_canvas_element_size("#canvas", &width, &height);

    int i = (int)((mouseEvent->targetX / (float)width) * N) + 1;
    int j = (int)(((height - mouseEvent->targetY) / (float)height) * N) + 1;

    if (isDragging && isValidGridCell(i, j)) {
        float velX = (mouseEvent->targetX - lastX) * 0.3f;
        float velY = (lastY - mouseEvent->targetY) * 0.3f;
        velX = std::min(std::max(velX, -10.0f), 10.0f);
        velY = std::min(std::max(velY, -10.0f), 10.0f);

        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                if (isValidGridCell(i + di, j + dj)) {
                    float factor = std::max(0.0f, 1.0f - 0.05f * (abs(di) + abs(dj)));
                    u_prev[IX(i + di, j + dj)] += velX * 10 * factor;
                    v_prev[IX(i + di, j + dj)] += velY * 10 * factor;
                    dens_prev[IX(i + di, j + dj)] += 60.0f * factor;
                }
            }
        }
    }

    lastX = mouseEvent->targetX;
    lastY = mouseEvent->targetY;
}

void setupInputCallbacks() {
    emscripten_set_mousedown_callback("#canvas", NULL, true, mouse_button_callback);
    emscripten_set_mouseup_callback("#canvas", NULL, true, mouse_button_callback);
    emscripten_set_mousemove_callback("#canvas", NULL, true, cursor_pos_callback);
}

void checkGLError(const std::string& place) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("OpenGL error at %s: %d\n", place.c_str(), err);
    }
}