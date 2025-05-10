#include "utils.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <iostream>
#include "fluid.hpp"

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

void checkGLError(const std::string& place) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << place << ": " << error << std::endl;
    }
}