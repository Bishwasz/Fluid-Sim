#include "fluid.hpp"
#include "utils.hpp"
#include <emscripten.h> // Added for emscripten_get_now
#include <algorithm>
#include <cmath>
#include <cstdio> // Added for printf

// Simulation parameters
float dt = 0.01f;
float diff = 0.0001f;
float visc = 0.001f;

// Fluid data
std::vector<float> u(SIZE), v(SIZE), u_prev(SIZE), v_prev(SIZE);
std::vector<float> dens(SIZE), dens_prev(SIZE);

void initFluid() {
    std::fill(u.begin(), u.end(), 0.0f);
    std::fill(v.begin(), v.end(), 0.0f);
    std::fill(dens.begin(), dens.end(), 0.0f);
    std::fill(u_prev.begin(), u_prev.end(), 0.0f);
    std::fill(v_prev.begin(), v_prev.end(), 0.0f);
    std::fill(dens_prev.begin(), dens_prev.end(), 0.0f);
}

void add_fixed_circular_source(std::vector<float>& dens_prev, 
                              std::vector<float>& u_prev, 
                              std::vector<float>& v_prev, 
                              float dt, float simulationTime) {
    float centerX = N * 0.5f + 1.0f;
    float centerY = N * 0.5f + 1.0f;
    float radius = 5.0f;
    float maxDensity = 500.0f;

    // Velocity parameters
    float velocityStrength = 50.0f;
    float rotationSpeed = 0.5f;
    float velocityDirection = fmod(simulationTime * rotationSpeed, 2.0f * M_PI);

    float velocityX = velocityStrength * std::cos(velocityDirection);
    float velocityY = velocityStrength * std::sin(velocityDirection);

    int minI = std::max(1, static_cast<int>(centerX - radius));
    int maxI = std::min(N, static_cast<int>(centerX + radius));
    int minJ = std::max(1, static_cast<int>(centerY - radius));
    int maxJ = std::min(N, static_cast<int>(centerY + radius));

    for (int i = minI; i <= maxI; i++) {
        for (int j = minJ; j <= maxJ; j++) {
            float dx = (i - centerX);
            float dy = (j - centerY);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance <= radius) {
                dens_prev[IX(i,j)] += maxDensity * dt;
                u_prev[IX(i,j)] += velocityX * dt;
                v_prev[IX(i,j)] += velocityY * dt;
            }
        }
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
    add_source(u, u0, dt);
    add_source(v, v0, dt);
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

float simulationTime = 0.0f;
void updateFluid(float dt) {
    simulationTime += dt;
    add_fixed_circular_source(dens_prev, u_prev, v_prev, dt, simulationTime);
    vel_step(u, v, u_prev, v_prev, visc, dt);
    dens_step(dens, dens_prev, u, v, diff, dt);
    std::fill(u_prev.begin(), u_prev.end(), 0.0f);
    std::fill(v_prev.begin(), v_prev.end(), 0.0f);
    std::fill(dens_prev.begin(), dens_prev.end(), 0.0f);
}