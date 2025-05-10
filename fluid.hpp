#ifndef FLUID_HPP
#define FLUID_HPP
#include <vector>

// Grid parameters
#define N 200
#define SIZE ((N+2)*(N+2))
#define IX(i,j) ((i)+(N+2)*(j))

// Simulation parameters
extern float dt;
extern float diff;
extern float visc;

// Fluid data
extern std::vector<float> u, v, u_prev, v_prev;
extern std::vector<float> dens, dens_prev;

void initFluid();
void updateFluid(float dt);
void add_fixed_circular_source(std::vector<float>& dens_prev, float dt);
void vel_step(std::vector<float>& u, std::vector<float>& v,
              std::vector<float>& u0, std::vector<float>& v0, float visc, float dt);
void dens_step(std::vector<float>& x, std::vector<float>& x0,
               std::vector<float>& u, std::vector<float>& v, float diff, float dt);

#endif