#ifndef UTILS_HPP
#define UTILS_HPP
#include <vector>
#include <string>

void SWAP(std::vector<float>& x0, std::vector<float>& x);
void set_bnd(int b, std::vector<float>& x);
void add_source(std::vector<float>& x, std::vector<float>& s, float dt);
void checkGLError(const std::string& place);

#endif