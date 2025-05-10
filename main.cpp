#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

// Grid parameters
#define N 200
#define SIZE ((N+2)*(N+2))
#define IX(i,j) ((i)+(N+2)*(j))

// Simulation parameters
float dt = 0.01f;
float diff = 0.0001f;
float visc = 0.0001f;

// Fluid data
std::vector<float> u(SIZE), v(SIZE), u_prev(SIZE), v_prev(SIZE);
std::vector<float> dens(SIZE), dens_prev(SIZE);

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

// Utility functions
void SWAP(std::vector<float>& x0, std::vector<float>& x) {
    std::swap(x0, x);
}

void set_bnd(int b, std::vector<float>& x) {
    for (int i = 1; i <= N; i++) {
        x[IX(0,i)] = b == 1 ? -x[IX(1,i)] : x[IX(1,i)];
        x[IX(N+1,i)] = b == 1 ? -x[IX(N,i)] : x[IX(N,i)];
        x[IX(i,0)] = b == 2 ? -x[IX(i,1)] : x[IX(i,1)];
        x[IX(i,N+1)] = b == 2 ? -x[IX(i,N)] : x[IX(i,N)];
    }
    x[IX(0,0)] = 0.5f * (x[IX(1,0)] + x[IX(0,1)]);
    x[IX(0,N+1)] = 0.5f * (x[IX(1,N+1)] + x[IX(0,N)]);
    x[IX(N+1,0)] = 0.5f * (x[IX(N,0)] + x[IX(N+1,1)]);
    x[IX(N+1,N+1)] = 0.5f * (x[IX(N,N+1)] + x[IX(N+1,N)]);
}

void add_source(std::vector<float>& x, std::vector<float>& s, float dt) {
    for (int i = 0; i < SIZE; i++) {
        x[i] += dt * s[i];
    }
}

void diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt) {
    float a = dt * diff * N * N;
    for (int k = 0; k < 20; k++) {
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                x[IX(i,j)] = (x0[IX(i,j)] + a * (x[IX(i-1,j)] + x[IX(i+1,j)] +
                    x[IX(i,j-1)] + x[IX(i,j+1)])) / (1 + 4 * a);
            }
        }
        set_bnd(b, x);
    }
}

void advect(int b, std::vector<float>& d, std::vector<float>& d0,
           std::vector<float>& u, std::vector<float>& v, float dt) {
    float dt0 = dt * N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            float x = i - dt0 * u[IX(i,j)];
            float y = j - dt0 * v[IX(i,j)];
            if (x < 0.5f) x = 0.5f;
            if (x > N + 0.5f) x = N + 0.5f;
            int i0 = (int)x;
            int i1 = i0 + 1;
            if (y < 0.5f) y = 0.5f;
            if (y > N + 0.5f) y = N + 0.5f;
            int j0 = (int)y;
            int j1 = j0 + 1;
            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;
            d[IX(i,j)] = s0 * (t0 * d0[IX(i0,j0)] + t1 * d0[IX(i0,j1)]) +
                        s1 * (t0 * d0[IX(i1,j0)] + t1 * d0[IX(i1,j1)]);
        }
    }
    set_bnd(b, d);
}

void project(std::vector<float>& u, std::vector<float>& v,
            std::vector<float>& p, std::vector<float>& div) {
    float h = 1.0f / N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            div[IX(i,j)] = -0.5f * h * (u[IX(i+1,j)] - u[IX(i-1,j)] +
                v[IX(i,j+1)] - v[IX(i,j-1)]);
            p[IX(i,j)] = 0;
        }
    }
    set_bnd(0, div);
    set_bnd(0, p);
    for (int k = 0; k < 20; k++) {
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                p[IX(i,j)] = (div[IX(i,j)] + p[IX(i-1,j)] + p[IX(i+1,j)] +
                    p[IX(i,j-1)] + p[IX(i,j+1)]) / 4;
            }
        }
        set_bnd(0, p);
    }
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            u[IX(i,j)] -= 0.5f * (p[IX(i+1,j)] - p[IX(i-1,j)]) / h;
            v[IX(i,j)] -= 0.5f * (p[IX(i,j+1)] - p[IX(i,j-1)]) / h;
        }
    }
    set_bnd(1, u);
    set_bnd(2, v);
}

void dens_step(std::vector<float>& x, std::vector<float>& x0,
              std::vector<float>& u, std::vector<float>& v, float diff, float dt) {
    add_source(x, x0, dt);
    SWAP(x0, x);
    diffuse(0, x, x0, diff, dt);
    SWAP(x0, x);
    advect(0, x, x0, u, v, dt);
}

void vel_step(std::vector<float>& u, std::vector<float>& v,
    std::vector<float>& u0, std::vector<float>& v0, float visc, float dt) {
    add_source(u, u0, dt); // Add external velocity source
    add_source(v, v0, dt); // Add external velocity source
    SWAP(u0, u);
    diffuse(1, u, u0, visc, dt);
    SWAP(v0, v);
    diffuse(2, v, v0, visc, dt);
    project(u, v, u0, v0);
    SWAP(u0, u);
    SWAP(v0, v);
    advect(1, u, u0, u, v, dt);
    advect(2, v, v0, u, v, dt);
    project(u, v, u0, v0);
}

// Check OpenGL errors
void checkGLError(const std::string& place) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << place << ": " << error << std::endl;
    }
}

// Shader compilation check
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

// OpenGL setup
bool initGL() {
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewStatus) << std::endl;
        return false;
    }
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Create and compile shaders
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

    // Check linking status
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

    // Setup VAO and VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    return true;
}

void updateVBO() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float scale = 2.0f / N;
    
    vertices.reserve(N * N * 5); // pre-allocate for better performance
    indices.reserve(N * N * 6);  // each quad has 2 triangles (6 indices)
    
    unsigned int vertexCount = 0;
    
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            float x = (i - 0.5f) * scale - 1.0f;
            float y = (j - 0.5f) * scale - 1.0f;
            float d = dens[IX(i,j)];
            d = std::min(std::max(d, 0.0f), 1.0f);

            // Create vertices for this cell
            // Bottom-left vertex
            vertices.push_back(x); vertices.push_back(y);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            // Bottom-right vertex
            vertices.push_back(x + scale); vertices.push_back(y);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            // Top-right vertex
            vertices.push_back(x + scale); vertices.push_back(y + scale);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            // Top-left vertex
            vertices.push_back(x); vertices.push_back(y + scale);
            vertices.push_back(d); vertices.push_back(d); vertices.push_back(d);
            
            // Create indices for this cell (2 triangles)
            indices.push_back(vertexCount);
            indices.push_back(vertexCount + 1);
            indices.push_back(vertexCount + 2);
            indices.push_back(vertexCount);
            indices.push_back(vertexCount + 2);
            indices.push_back(vertexCount + 3);
            
            vertexCount += 4;
        }
    }

    // Update VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_DYNAMIC_DRAW);
                 
    // Update EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_DYNAMIC_DRAW);

    // Configure vertex attributes
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
    
    // Draw all quads with a single draw call
    glDrawElements(GL_TRIANGLES, N * N * 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    checkGLError("render");
}

// Safe array access with bounds checking
bool isValidGridCell(int i, int j) {
    return (i >= 1 && i <= N && j >= 1 && j <= N);
}

// Mouse interaction
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
            
            // Add some density to surrounding cells for a smoother appearance
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
        velX = fminf(fmaxf(velX, -10.0f), 10.0f);
        velY = fminf(fmaxf(velY, -10.0f), 10.0f);

        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                if (isValidGridCell(i+di, j+dj)) {
                    float factor = fmaxf(0.0f, 1.0f - 0.05f * (abs(di) + abs(dj)));
                    u_prev[IX(i+di, j+dj)] += velX * 10 * factor;
                    v_prev[IX(i+di, j+dj)] += velY * 10 * factor;
                    dens_prev[IX(i+di, j+dj)] += 50.0f * factor; // Add density while dragging
                }
            }
        }
    }

    lastX = xpos;
    lastY = ypos;
}
void add_fixed_circular_source(std::vector<float>& dens_prev, float dt) {
    // Fixed source parameters
    float centerX = N * 0.5f + 1.0f; // Center of the grid (x-coordinate)
    float centerY = N * 0.5f + 1.0f; // Center of the grid (y-coordinate)
    float radius = 5.0f; // Radius in grid units
    float maxDensity = 100.0f; // Maximum density at center, scaled by dt

    // Iterate over a square region around the source to check for cells within the circle
    int minI = std::max(1, static_cast<int>(centerX - radius));
    int maxI = std::min(N, static_cast<int>(centerX + radius));
    int minJ = std::max(1, static_cast<int>(centerY - radius));
    int maxJ = std::min(N, static_cast<int>(centerY + radius));

    for (int i = minI; i <= maxI; i++) {
        for (int j = minJ; j <= maxJ; j++) {
            // Calculate distance from grid cell center to source center
            float dx = (i - centerX);
            float dy = (j - centerY);
            float distance = std::sqrt(dx * dx + dy * dy);

            // Add density if within radius
            if (distance <= radius) {
                // Gaussian falloff for smooth distribution
                float falloff = std::exp(-distance * distance / (radius * radius));
                dens_prev[IX(i,j)] += maxDensity * falloff * dt;
            }
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Request OpenGL 3.3 core profile
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
    
    // Initialize OpenGL
    if (!initGL()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return -1;
    }

    // Setup mouse callbacks
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    // Initialize fluid
    std::fill(u.begin(), u.end(), 0.0f);
    std::fill(v.begin(), v.end(), 0.0f);
    std::fill(dens.begin(), dens.end(), 0.0f);
    std::fill(u_prev.begin(), u_prev.end(), 0.0f);
    std::fill(v_prev.begin(), v_prev.end(), 0.0f);
    std::fill(dens_prev.begin(), dens_prev.end(), 0.0f);
    float gravity = -0.5f;
    // Main loop
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        // Calculate frame time
        double currentTime = glfwGetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Update simulation with adjusted timestep to ensure stability
        float adjustedDt = std::min(dt, float(frameTime * 5.0));
        add_fixed_circular_source(dens_prev, adjustedDt);


        

        
        vel_step(u, v, u_prev, v_prev, visc, adjustedDt);
        dens_step(dens, dens_prev, u, v, diff, adjustedDt);

        // Clear previous sources
        std::fill(u_prev.begin(), u_prev.end(), 0.0f);
        std::fill(v_prev.begin(), v_prev.end(), 0.0f);
        std::fill(dens_prev.begin(), dens_prev.end(), 0.0f);

        // Update and render
        updateVBO();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Check for errors
        checkGLError("main loop");
    }

    // Cleanup
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}