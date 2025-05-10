#include "render.hpp"
#include "fluid.hpp"
#include <vector>
#include <iostream>
#include <algorithm>

// OpenGL variables
GLuint vao, vbo, ebo, shaderProgram;
GLint posAttrib, colorAttrib;

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec3 color;
    out vec3 fragColor;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        fragColor = color;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
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
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return false;
    }
    return true;
}

bool initGL() {
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewStatus) << std::endl;
        return false;
    }
    
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
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        int i = (int)((xpos / width) * N) + 1;
        int j = (int)(((height - ypos) / height) * N) + 1;

        if (isValidGridCell(i, j)) {
            dens_prev[IX(i,j)] = 100.0f;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    if (isValidGridCell(i+di, j+dj)) {
                        dens_prev[IX(i+di, j+dj)] += 60.0f * (1.0f - 0.3f * (abs(di) + abs(dj)));
                    }
                }
            }
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = xpos, lastY = ypos;
    static bool firstMove = true;

    if (firstMove) {
        firstMove = false;
        lastX = xpos;
        lastY = ypos;
        return;
    }

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    int i = (int)((xpos / width) * N) + 1;
    int j = (int)(((height - ypos) / height) * N) + 1;

    if (isValidGridCell(i, j) && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        float velX = (xpos - lastX) * 0.3f;
        float velY = (lastY - ypos) * 0.3f;
        velX = std::min(std::max(velX, -10.0f), 10.0f);
        velY = std::min(std::max(velY, -10.0f), 10.0f);

        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                if (isValidGridCell(i+di, j+dj)) {
                    float factor = std::max(0.0f, 1.0f - 0.05f * (abs(di) + abs(dj)));
                    u_prev[IX(i+di, j+dj)] += velX * 10 * factor;
                    v_prev[IX(i+di, j+dj)] += velY * 10 * factor;
                    dens_prev[IX(i+di, j+dj)] += 50.0f * factor;
                }
            }
        }
    }

    lastX = xpos;
    lastY = ypos;
}

void setupInputCallbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
}